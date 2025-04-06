using System;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Newtonsoft.Json;
using System.IO;


namespace sik
{
    public enum CommandType
    {
        None,
        VolumeUp,
        VolumeDown,
        BrightnessUp,
        BrightnessDown,
        OpenBrowser,
        CloseApp,
        LockScreen,
        AltTab
    }

    public class Command
    {
        public string Name { get; set; }
        public CommandType Type { get; set; }
    }

    public partial class Form1 : Form
    {

        Command[] buttonAssignments = new Command[8]; // 8 buton varsa
        SerialPort serial;
        int currentBrightness = 50;

        public Form1()
        {
            InitializeComponent();

            serial = new SerialPort("COM7", 9600); // HC-05 eşleştiği port
            serial.DataReceived += Serial_DataReceived;

            if (!File.Exists("buttons.json"))
            {
                buttonAssignments = new Command[8];
                for (int i = 0; i < buttonAssignments.Length; i++)
                {
                    buttonAssignments[i] = new Command { Name = "Yok", Type = CommandType.None };
                }
            }
            else
            {
                LoadAssignments();
            }

            try
            {
                serial.Open();
                MessageBox.Show("Bluetooth bağlantısı başarılı!");
            }
            catch (Exception ex)
            {
                MessageBox.Show("Bağlantı hatası: " + ex.Message);
            }
        }
        private void SaveAssignments()
        {
            string json = JsonConvert.SerializeObject(buttonAssignments);
            File.WriteAllText("buttons.json", json);
        }

        private void LoadAssignments()
        {
            if (File.Exists("buttons.json"))
            {
                string json = File.ReadAllText("buttons.json");
                buttonAssignments = JsonConvert.DeserializeObject<Command[]>(json);
            }
        }

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string incoming = serial.ReadLine().Trim();
            this.Invoke(new Action(() =>
            {
                HandleCommand(incoming);
            }));
        }

        private void HandleCommand(string cmd)
        {
            Console.WriteLine("Gelen komut: " + cmd);

            if (cmd.StartsWith("CMD:BTN"))
            {
                int index = int.Parse(cmd.Substring(7)); // Örn: CMD:BTN3 → 3
                if (index >= 0 && index < buttonAssignments.Length)
                {
                    ExecuteCommand(buttonAssignments[index]);
                }
            }
        }

        private void SendKey(Keys key)
        {
            keybd_event((byte)key, 0, 0, 0);
            keybd_event((byte)key, 0, 2, 0);
        }

        private void SendAltTab()
        {
            keybd_event((byte)Keys.Menu, 0, 0, 0);
            keybd_event((byte)Keys.Tab, 0, 0, 0);
            keybd_event((byte)Keys.Tab, 0, 2, 0);
            keybd_event((byte)Keys.Menu, 0, 2, 0);
        }

        private void AdjustBrightness(int change)
        {
            currentBrightness = Math.Max(0, Math.Min(100, currentBrightness + change));
            SetBrightness(currentBrightness);
        }

        private void SetBrightness(int brightness)
        {
            string cmd = $"(Get-WmiObject -Namespace root/WMI -Class WmiMonitorBrightnessMethods).WmiSetBrightness(1, {brightness})";
            System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "powershell",
                Arguments = $"-Command \"{cmd}\"",
                UseShellExecute = false,
                CreateNoWindow = true
            });
        }

        [DllImport("user32.dll")] static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, uint dwExtraInfo);
        [DllImport("user32.dll")] static extern bool LockWorkStation();

        private void Form1_Load(object sender, EventArgs e) { }

        private void ExecuteCommand(Command cmd)
        {
            switch (cmd.Type)
            {
                case CommandType.VolumeUp: SendKey(Keys.VolumeUp); break;
                case CommandType.VolumeDown: SendKey(Keys.VolumeDown); break;
                case CommandType.BrightnessUp: AdjustBrightness(10); break;
                case CommandType.BrightnessDown: AdjustBrightness(-10); break;
                case CommandType.OpenBrowser: System.Diagnostics.Process.Start("https://www.google.com"); break;
                case CommandType.CloseApp: Application.Exit(); break;
                case CommandType.LockScreen: LockWorkStation(); break;
                case CommandType.AltTab: SendAltTab(); break;
            }
        }
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            SaveAssignments();
        }
    }

}
