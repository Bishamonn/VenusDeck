using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Management;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Linq;

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
        AltTab,
        CustomAction
    }

    public partial class Form1 : Form
    {
        SerialPort serial;
        int currentBrightness;
        private Dictionary<string, CommandType> buttonMappings;
        private Dictionary<string, Label> buttonLabels; // Buton etiketlerini tutacak sözlük
        private string configPath = "buttonMappings.json";
        private string buttonLabelsPath = "buttonLabels.json"; // Etiketler için ayrı config

        // Komut türlerine karşılık gelen etiketler
        private readonly Dictionary<CommandType, string> commandLabels = new Dictionary<CommandType, string>
        {
            { CommandType.None, "BOŞ" },
            { CommandType.VolumeUp, "SES+" },
            { CommandType.VolumeDown, "SES-" },
            { CommandType.BrightnessUp, "PRLK+" },
            { CommandType.BrightnessDown, "PRLK-" },
            { CommandType.OpenBrowser, "GOOGLE" },
            { CommandType.CloseApp, "KAPAT" },
            { CommandType.LockScreen, "KİLİT" },
            { CommandType.AltTab, "ALT+TAB" },
            { CommandType.CustomAction, "ÖZEL" }
        };

        public Form1()
        {

            // Form'un boyutlarını içeriğe göre otomatik ayarla
            this.AutoSize = true;
            this.AutoSizeMode = AutoSizeMode.GrowAndShrink;

            // Minimum boyutu ayarla
            this.MinimumSize = new System.Drawing.Size(400, 450);

            // Form'u ekranın ortasında başlat
            this.StartPosition = FormStartPosition.CenterScreen;


            InitializeComponent();

            // 1) Sözlükleri oluştur
            buttonMappings = new Dictionary<string, CommandType>();
            buttonLabels = new Dictionary<string, Label>();

            // 2) Kaydedilmiş config'i yükle
            LoadButtonMappings();

            // 3) Dinamik UI'yi oluştur
            InitializeDynamicUI();

            // 4) Mevcut parlaklığı al
            currentBrightness = GetCurrentBrightness();

            // 5) Seri port ayarları
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

            // Form yüklendiğinde ComboBox'ları ve etiketleri ayarla
            SetComboBoxValues();
            UpdateAllButtonLabels();
        }

        // ComboBox'ları buttonMappings değerlerine göre ayarlama
        private void SetComboBoxValues()
        {
            for (int i = 1; i <= 9; i++)
            {
                int index = i;
                string buttonName = $"Button{index}";
                if (buttonMappings.TryGetValue(buttonName, out var command))
                {
                    var comboBox = this.Controls.Find($"comboBox{index}", true);
                    if (comboBox.Length > 0 && comboBox[0] is ComboBox cb)
                    {
                        cb.SelectedItem = command;
                        Console.WriteLine($"ComboBox {index} ayarlandı: {command}");
                    }
                }
            }
        }

        // Tüm butonların etiketlerini güncelle
        private void UpdateAllButtonLabels()
        {
            foreach (var pair in buttonMappings)
            {
                string buttonName = pair.Key;
                CommandType command = pair.Value;

                // Button1 -> 1 formatında indekse dönüştür 
                if (int.TryParse(buttonName.Replace("Button", ""), out int buttonIndex))
                {
                    UpdateButtonLabel(buttonIndex, command);
                }
            }
        }

        // Belirli bir butonun etiketini güncelle
        private void UpdateButtonLabel(int buttonIndex, CommandType command)
        {
            // İlgili Label'ı bul
            if (buttonLabels.TryGetValue($"Button{buttonIndex}", out var label))
            {
                // Komuta karşılık gelen etiketi bul
                if (commandLabels.TryGetValue(command, out string labelText))
                {
                    label.Text = $"Buton {buttonIndex}: {labelText}";
                }
                else
                {
                    label.Text = $"Buton {buttonIndex}: ???";
                }
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Form_Load'da tekrar ComboBox değerlerini ve etiketleri ayarla
            SetComboBoxValues();
            UpdateAllButtonLabels();
        }

        private void InitializeDynamicUI()
        {
            for (int i = 1; i <= 9; i++)
            {
                int index = i;

                // Label oluştur
                Label label = new Label
                {
                    Name = $"label{index}",
                    Text = $"Buton {index}:",
                    Location = new System.Drawing.Point(20, 30 * index),
                    AutoSize = true
                };
                this.Controls.Add(label);

                // Label'ı sözlüğe kaydet
                buttonLabels[$"Button{index}"] = label;

                // ComboBox oluştur
                ComboBox comboBox = new ComboBox
                {
                    Name = $"comboBox{index}",
                    Location = new System.Drawing.Point(150, 30 * index),
                    Width = 150,
                    DropDownStyle = ComboBoxStyle.DropDownList
                };
                comboBox.DataSource = Enum.GetValues(typeof(CommandType));

                // Burada SelectedItem değerini ayarla
                if (buttonMappings.TryGetValue($"Button{index}", out var command))
                {
                    comboBox.SelectedItem = command;
                }

                // ComboBox değişiklik eventi
                comboBox.SelectedIndexChanged += (sender, e) =>
                {
                    if (comboBox.SelectedItem is CommandType selectedCommand)
                    {
                        buttonMappings[$"Button{index}"] = selectedCommand;
                        UpdateButtonLabel(index, selectedCommand); // Etiketi güncelle
                    }
                };
                this.Controls.Add(comboBox);
            }

            // Son olarak "Kaydet" butonu ekleyelim
            Button btnSave = new Button
            {
                Text = "Değişiklikleri Kaydet",
                Location = new System.Drawing.Point(20, 350),
                Width = 230
            };
            btnSave.Click += (sender, e) => SaveButtonMappings();
            this.Controls.Add(btnSave);

            // Arduino'ya Gönder butonu
            Button btnSendToArduino = new Button
            {
                Text = "Arduino'ya Gönder",
                Location = new System.Drawing.Point(20, 390),
                Width = 230
            };
            btnSendToArduino.Click += (sender, e) => SendConfigToArduino();
            this.Controls.Add(btnSendToArduino);
        }

        // Remove one of the duplicate SendConfigToArduino methods
        private void SendConfigToArduino()
        {
            if (serial == null || !serial.IsOpen)
            {
                MessageBox.Show("Arduino ile bağlantı yok!");
                return;
            }

            try
            {
                // Her buton için ayrı bir CFG komutu gönder
                foreach (var pair in buttonMappings)
                {
                    // Button1 -> 1 formatında indekse dönüştür 
                    if (int.TryParse(pair.Key.Replace("Button", ""), out int buttonIndex))
                    {
                        // CFG:ButtonIndex:CommandCode formatında komut oluştur
                        string command = $"CFG:{buttonIndex}:{pair.Value}";

                        // Komutu gönder
                        serial.WriteLine(command);
                        Console.WriteLine($"Arduino'ya gönderilen komut: {command}");

                        // Arduino'nun cevabını bekleyin (opsiyonel)
                        System.Threading.Thread.Sleep(100);
                    }
                }

                MessageBox.Show("Tüm buton ayarları Arduino'ya gönderildi!");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Arduino'ya gönderim hatası: {ex.Message}");
            }
        }




        private void InitializeButtonMappings()
        {
            buttonMappings.Clear();

            // Butonlar için varsayılan değerler
            buttonMappings["Button1"] = CommandType.VolumeDown;
            buttonMappings["Button2"] = CommandType.VolumeUp;
            buttonMappings["Button3"] = CommandType.BrightnessDown;
            buttonMappings["Button4"] = CommandType.BrightnessUp;
            buttonMappings["Button5"] = CommandType.OpenBrowser;
            buttonMappings["Button6"] = CommandType.CloseApp;
            buttonMappings["Button7"] = CommandType.LockScreen;
            buttonMappings["Button8"] = CommandType.AltTab;
            buttonMappings["Button9"] = CommandType.CustomAction;
        }

        private void LoadButtonMappings()
        {
            // Dosya yoksa varsayılan değerlerle oluştur
            if (!File.Exists(configPath))
            {
                InitializeButtonMappings();
                SaveButtonMappings();
                return;
            }

            try
            {
                // Dosya varsa oku
                string json = File.ReadAllText(configPath);
                Console.WriteLine("Yüklenen JSON: " + json);

                // Önce JObject olarak parse et
                JObject jsonObject = JObject.Parse(json);

                // Yeni sözlük oluştur
                buttonMappings.Clear();

                // Button1-Button9 için değerleri yükle
                for (int i = 1; i <= 9; i++)
                {
                    string buttonName = $"Button{i}";
                    if (jsonObject.ContainsKey(buttonName))
                    {
                        // Tırnak işaretlerini kaldır
                        string enumValueStr = jsonObject[buttonName].ToString().Trim('"');
                        if (Enum.TryParse<CommandType>(enumValueStr, out var command))
                        {
                            buttonMappings[buttonName] = command;
                            Console.WriteLine($"Yüklendi: {buttonName} -> {command}");
                        }
                        else
                        {
                            buttonMappings[buttonName] = CommandType.None;
                            Console.WriteLine($"Parse Hatası: {buttonName} -> {enumValueStr}");
                        }
                    }
                    else
                    {
                        buttonMappings[buttonName] = CommandType.None;
                        Console.WriteLine($"Key Bulunamadı: {buttonName}");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Config dosyası yüklenirken hata oluştu: {ex.Message}\nVarsayılan değerler kullanılacak.");
                InitializeButtonMappings();
                SaveButtonMappings();
            }
        }

        private void SaveButtonMappings()
        {
            try
            {
                var jsonObject = new JObject();
                foreach (var pair in buttonMappings)
                {
                    // Enum değerini doğrudan string olarak ekle
                    jsonObject[pair.Key] = pair.Value.ToString();
                }

                // JSON'ı formatlı hale getir ve kaydet
                string json = jsonObject.ToString(Formatting.Indented);
                File.WriteAllText(configPath, json);

                Console.WriteLine("Kaydedilen JSON: " + json);
                MessageBox.Show("Değişiklikler başarıyla kaydedildi!");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Config dosyası kaydedilirken hata oluştu: {ex.Message}");
            }
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
                    return Convert.ToInt32(mo["CurrentBrightness"]);
                }
            }
            catch { }
            return 50;
        }

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string cmd = serial.ReadLine().Trim();
            this.Invoke(new Action(() => HandleCommand(cmd)));
        }

        private void HandleCommand(string cmd)
        {
            Console.WriteLine($"Gelen komut: {cmd}");

            if (!cmd.StartsWith("CMD: ")) return;
            string rawCommandName = cmd.Substring(5);

            string buttonName = MapCommandToButton(rawCommandName);
            if (string.IsNullOrEmpty(buttonName))
            {
                Console.WriteLine($"❓ Tanınmayan komut: {rawCommandName}");
                MessageBox.Show("Tanımlanmamış Komut: " + rawCommandName);
                return;
            }

            if (buttonMappings.TryGetValue(buttonName, out var command))
            {
                ExecuteCommand(command);
            }
            else
            {
                Console.WriteLine($"❓ Tanınmayan buton: {buttonName}");
                MessageBox.Show("Tanımlanmamış Buton: " + buttonName);
            }
        }

        private string MapCommandToButton(string commandName)
        {
            switch (commandName)
            {
                case "VOLUME_DOWN": return "Button1";
                case "VOLUME_UP": return "Button2";
                case "BRIGHTNESS_DOWN": return "Button3";
                case "BRIGHTNESS_UP": return "Button4";
                case "OPEN_BROWSER": return "Button5";
                case "CLOSE_APP": return "Button6";
                case "LOCK_SCREEN": return "Button7";
                case "ALT_TAB": return "Button8";
                case "CUSTOM_ACTION": return "Button9";
                default: return null;
            }
        }

        private void ExecuteCommand(CommandType ct)
        {
            switch (ct)
            {
                case CommandType.VolumeUp:
                    SendKey(Keys.VolumeUp);
                    break;
                case CommandType.VolumeDown:
                    SendKey(Keys.VolumeDown);
                    break;
                case CommandType.BrightnessUp:
                    AdjustBrightness(+10);
                    break;
                case CommandType.BrightnessDown:
                    AdjustBrightness(-10);
                    break;
                case CommandType.OpenBrowser:
                    System.Diagnostics.Process.Start("https://www.google.com");
                    break;
                case CommandType.CloseApp:
                    Application.Exit();
                    break;
                case CommandType.LockScreen:
                    LockWorkStation();
                    break;
                case CommandType.AltTab:
                    SendAltTab();
                    break;
                case CommandType.CustomAction:
                    MessageBox.Show("Özel İşlem Yapıldı!");
                    break;
                case CommandType.None:
                default:
                    MessageBox.Show("Hiçbir komut atanmamış!");
                    break;
            }
        }

        private void SendKey(Keys k)
        {
            keybd_event((byte)k, 0, 0, 0);
            keybd_event((byte)k, 0, 2, 0);
        }

        private void SendAltTab()
        {
            keybd_event((byte)Keys.Menu, 0, 0, 0);
            keybd_event((byte)Keys.Tab, 0, 0, 0);
            keybd_event((byte)Keys.Tab, 0, 2, 0);
            keybd_event((byte)Keys.Menu, 0, 2, 0);
        }

        private void AdjustBrightness(int delta)
        {
            currentBrightness = Math.Max(0, Math.Min(100, currentBrightness + delta));
            SetBrightness(currentBrightness);
        }

        private void SetBrightness(int brightness)
        {
            string ps = "(Get-WmiObject -Namespace root/WMI -Class WmiMonitorBrightnessMethods)"
                      + $".WmiSetBrightness(1,{brightness})";

            System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "powershell",
                Arguments = $"-Command \"{ps}\"",
                UseShellExecute = false,
                CreateNoWindow = true
            });
        }

        [DllImport("user32.dll")]
        static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, uint dwExtraInfo);

        [DllImport("user32.dll")]
        static extern bool LockWorkStation();
    }
}
