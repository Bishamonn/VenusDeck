using System;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Management;

namespace VenusDeck
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

    public partial class Form1 : Form
    {
        SerialPort serial;
        int currentBrightness;
        int currentPage = 1; // 🔄 Sayfa takibi

        CommandType[] commandMappingsPage1 = new CommandType[8]
        {
            CommandType.VolumeDown,
            CommandType.VolumeUp,
            CommandType.BrightnessDown,
            CommandType.BrightnessUp,
            CommandType.OpenBrowser,
            CommandType.CloseApp,
            CommandType.LockScreen,
            CommandType.AltTab
        };

        public Form1()
        {
            InitializeComponent();

            currentBrightness = GetCurrentBrightness();

            serial = new SerialPort("COM7", 9600);
            serial.DataReceived += Serial_DataReceived;

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
        private void Form1_Load(object sender, System.EventArgs e)
        {

        }

        private int GetCurrentBrightness()
        {
            try
            {
                var mclass = new ManagementClass("WmiMonitorBrightness");
                mclass.Scope = new ManagementScope(@"\\.\root\wmi");
                foreach (var o in mclass.GetInstances())
                {
                    var mo = (ManagementObject)o;
                    return Convert.ToInt32(mo.GetPropertyValue("CurrentBrightness"));
                }
            }
            catch
            {
                return 50;
            }

            return 50;
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
            Console.WriteLine($"Sayfa {currentPage} - Gelen komut: {cmd}");

            // 1) Sayfa değişim komutları
            if (cmd == "CMD: >")
            {
                currentPage = 2;
                Console.WriteLine("👉 Sayfa 2'ye geçildi.");
                return;
            }
            if (cmd == "CMD: <")
            {
                currentPage = 1;
                Console.WriteLine("👈 Sayfa 1'e dönüldü.");
                return;
            }

            // 2) Birinci sayfaysa sadece birinci sayfa komutlarını işle
            if (currentPage == 1)
            {
                const string prefix = "CMD: ";
                if (!cmd.StartsWith(prefix)) return;  // prefix yoksa tamamen yok say

                string name = cmd.Substring(prefix.Length).Trim().Replace("_", "");

                if (Enum.TryParse<CommandType>(name, true, out var ct))
                {
                    ExecuteCommand(ct);
                }
                else
                {
                    Console.WriteLine("❓ Sayfa1 tanınmayan komut: " + name);
                }
            }
            // 3) İkinci sayfaysa sadece ikinci sayfa komutlarını işle
            else if (currentPage == 2)
            {
                switch (cmd)
                {
                    case "CMD: BTN0": MessageBox.Show("Sayfa 2 - Buton 1"); break;
                    case "CMD: BTN1": MessageBox.Show("Sayfa 2 - Buton 2"); break;
                    case "CMD: BTN2": MessageBox.Show("Sayfa 2 - Buton 3"); break;
                    case "CMD: BTN3": MessageBox.Show("Sayfa 2 - Buton 4"); break;
                    case "CMD: BTN4": MessageBox.Show("Sayfa 2 - Buton 5"); break;
                    case "CMD: BTN5": MessageBox.Show("Sayfa 2 - Buton 6"); break;
                    case "CMD: BTN6": MessageBox.Show("Sayfa 2 - Buton 7"); break;
                    case "CMD: BTN7": MessageBox.Show("Sayfa 2 - Buton 8"); break;
                    default: Console.WriteLine("Sayfa 2'de bilinmeyen komut: " + cmd); break;
                }
            }
        }

        private void ExecuteCommand(CommandType type)
        {
            switch (type)
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
    }
}
