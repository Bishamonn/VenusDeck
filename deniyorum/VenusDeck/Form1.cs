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
using System.Linq;

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
        private Dictionary<string, Dictionary<string, CommandType>> profiles;
        private string currentProfileName = "Default";
        private ComboBox profileComboBox;
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
        private bool isInitializing = false;

        public Form1()
        {
            // Form'un boyutlarını içeriğe göre otomatik ayarla
            this.AutoSize = true;
            this.AutoSizeMode = AutoSizeMode.GrowAndShrink;

            // Minimum boyutu ayarla
            this.MinimumSize = new System.Drawing.Size(400, 450);

            // Form'u ekranın ortasında başlat
            this.StartPosition = FormStartPosition.CenterScreen;

            // Arka plan rengini ayarla
            this.BackColor = System.Drawing.ColorTranslator.FromHtml("#494E55");

            InitializeComponent();

            // 1) Sözlükleri oluştur
            profiles = new Dictionary<string, Dictionary<string, CommandType>>();
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
            isInitializing = true;
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
            isInitializing = false;
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
            // Profil seçimi için ComboBox ekle
            Label profileLabel = new Label
            {
                Text = "Profil:",
                Location = new System.Drawing.Point(20, 10),
                AutoSize = true,
                ForeColor = System.Drawing.Color.White
            };
            this.Controls.Add(profileLabel);

            profileComboBox = new ComboBox
            {
                Name = "profileComboBox",
                Location = new System.Drawing.Point(150, 10),
                Width = 150,
                DropDownStyle = ComboBoxStyle.DropDownList
            };

            // Profil listesini doldur
            foreach (var profileName in profiles.Keys)
            {
                profileComboBox.Items.Add(profileName);
            }

            // Mevcut profili seç
            if (profiles.ContainsKey(currentProfileName))
            {
                profileComboBox.SelectedItem = currentProfileName;
            }
            else if (profileComboBox.Items.Count > 0)
            {
                profileComboBox.SelectedIndex = 0;
                currentProfileName = profileComboBox.SelectedItem.ToString();
            }

            profileComboBox.SelectedIndexChanged += ProfileComboBox_SelectedIndexChanged;
            this.Controls.Add(profileComboBox);

            // Yeni profil eklemek için buton
            Button btnNewProfile = new Button
            {
                Text = "Yeni Profil",
                Location = new System.Drawing.Point(310, 10),
                Width = 100
            };
            btnNewProfile.Click += BtnNewProfile_Click;
            this.Controls.Add(btnNewProfile);

            for (int i = 1; i <= 9; i++)
            {
                int index = i;

                // Label oluştur
                Label label = new Label
                {
                    Name = $"label{index}",
                    Text = $"Buton {index}:",
                    Location = new System.Drawing.Point(20, 40 + 30 * index),
                    AutoSize = true,
                    ForeColor = System.Drawing.Color.White
                };
                this.Controls.Add(label);

                // Label'ı sözlüğe kaydet
                buttonLabels[$"Button{index}"] = label;

                // ComboBox oluştur
                ComboBox comboBox = new ComboBox
                {
                    Name = $"comboBox{index}",
                    Location = new System.Drawing.Point(150, 40 + 30 * index),
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
                    if (isInitializing) return; // Sadece kullanıcı değişikliklerinde çalışsın

                    if (comboBox.SelectedItem is CommandType selectedCommand)
                    {
                        buttonMappings[$"Button{index}"] = selectedCommand;
                        UpdateButtonLabel(index, selectedCommand);
                        SendConfigToArduino();
                    }
                };
                this.Controls.Add(comboBox);
            }

            // Son olarak "Kaydet" butonu ekleyelim
            Button btnSave = new Button
            {
                Text = "Değişiklikleri Kaydet",
                Location = new System.Drawing.Point(20, 40 + 9 * 30 + 20),
                Width = 230
            };
            btnSave.Click += (sender, e) => SaveButtonMappings();
            this.Controls.Add(btnSave);

            // Arduino'ya Gönder butonu
            Button btnSendToArduino = new Button
            {
                Text = "Arduino'ya Gönder",
                Location = new System.Drawing.Point(20, 40 + 9 * 30 + 60),
                Width = 230
            };
            btnSendToArduino.Click += (sender, e) => SendConfigToArduino();
            this.Controls.Add(btnSendToArduino);
        }

        private void ProfileComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (profileComboBox.SelectedItem != null)
            {
                // Mevcut profildeki değişiklikleri otomatik kaydet
                SaveCurrentProfileSettings();

                // Yeni profili yükle
                currentProfileName = profileComboBox.SelectedItem.ToString();
                LoadProfileSettings(currentProfileName);

                // UI'yi güncelle
                SetComboBoxValues();
                UpdateAllButtonLabels();
            }
        }

        private void BtnNewProfile_Click(object sender, EventArgs e)
        {
            // Form oluştur
            Form inputForm = new Form()
            {
                Width = 300,
                Height = 150,
                FormBorderStyle = FormBorderStyle.FixedDialog,
                Text = "Yeni Profil",
                StartPosition = FormStartPosition.CenterParent
            };

            // Etiket oluştur
            Label label = new Label() { Left = 20, Top = 20, Text = "Yeni profil adını giriniz:" };

            // Metin kutusu oluştur
            TextBox textBox = new TextBox() { Left = 20, Top = 50, Width = 250 };
            textBox.Text = "Profil " + (profiles.Count + 1);

            // OK butonu oluştur
            Button btnOk = new Button() { Text = "Tamam", Left = 130, Width = 70, Top = 80 };
            btnOk.Click += (s, evt) => { inputForm.DialogResult = DialogResult.OK; };

            // Formu oluştur
            inputForm.Controls.Add(label);
            inputForm.Controls.Add(textBox);
            inputForm.Controls.Add(btnOk);
            inputForm.AcceptButton = btnOk;

            // Formu göster
            if (inputForm.ShowDialog() == DialogResult.OK)
            {
                string newProfileName = textBox.Text;
                if (!string.IsNullOrWhiteSpace(newProfileName))
                {
                    // Profil zaten var mı kontrol et
                    if (profiles.ContainsKey(newProfileName))
                    {
                        MessageBox.Show("Bu isimde bir profil zaten var!", "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }

                    // Yeni boş profil oluştur
                    Dictionary<string, CommandType> newProfile = new Dictionary<string, CommandType>();
                    for (int i = 1; i <= 9; i++)
                    {
                        newProfile[$"Button{i}"] = CommandType.None;
                    }

                    // Profiller sözlüğüne ekle
                    profiles[newProfileName] = newProfile;

                    // ComboBox'a ekle ve seç
                    profileComboBox.Items.Add(newProfileName);
                    profileComboBox.SelectedItem = newProfileName;

                    // Profili kaydet
                    SaveButtonMappings();
                }
            }
        }

        private void SaveCurrentProfileSettings()
        {
            // Mevcut ayarları profile kaydet
            if (!string.IsNullOrEmpty(currentProfileName))
            {
                profiles[currentProfileName] = new Dictionary<string, CommandType>(buttonMappings);
            }
        }

        private void LoadProfileSettings(string profileName)
        {
            if (profiles.ContainsKey(profileName))
            {
                // Buton eşlemelerini profil ayarlarından güncelle
                buttonMappings = new Dictionary<string, CommandType>(profiles[profileName]);
            }
        }


        private void SendConfigToArduino()
        {
            if (serial == null || !serial.IsOpen)
            {
                MessageBox.Show("Arduino ile bağlantı yok!");
                return;
            }

            try
            {
                // Her zaman 1'den 9'a sırayla butonları gönder
                for (int i = 1; i <= 9; i++)
                {
                    string buttonName = $"Button{i}";
                    if (buttonMappings.TryGetValue(buttonName, out var command))
                    {
                        // CFG:ButtonIndex:CommandCode formatında komut oluştur
                        string commandStr = $"CFG:{i}:{command}";

                        // Komutu gönder
                        serial.WriteLine(commandStr);
                        Console.WriteLine($"Arduino'ya gönderilen komut: {commandStr}");

                        // Arduino'nun cevabını bekle
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

        private void InitializeDefaultProfiles()
        {
            // Varsayılan profili oluştur
            Dictionary<string, CommandType> defaultProfile = new Dictionary<string, CommandType>();

            // Butonlar için varsayılan değerler
            defaultProfile["Button1"] = CommandType.VolumeDown;
            defaultProfile["Button2"] = CommandType.VolumeUp;
            defaultProfile["Button3"] = CommandType.BrightnessDown;
            defaultProfile["Button4"] = CommandType.BrightnessUp;
            defaultProfile["Button5"] = CommandType.OpenBrowser;
            defaultProfile["Button6"] = CommandType.CloseApp;
            defaultProfile["Button7"] = CommandType.LockScreen;
            defaultProfile["Button8"] = CommandType.AltTab;
            defaultProfile["Button9"] = CommandType.CustomAction;

            profiles["Default"] = defaultProfile;
            currentProfileName = "Default";
            buttonMappings = new Dictionary<string, CommandType>(defaultProfile);
        }

        private void LoadButtonMappings()
        {
            // Varsayılan profilleri oluştur
            profiles = new Dictionary<string, Dictionary<string, CommandType>>();

            // Dosya yoksa varsayılan değerlerle oluştur
            if (!File.Exists(configPath))
            {
                InitializeDefaultProfiles();
                SaveButtonMappings();
                return;
            }

            try
            {
                // Dosya varsa oku
                string json = File.ReadAllText(configPath);
                Console.WriteLine("Yüklenen JSON: " + json);

                // JObject olarak parse et
                JObject jsonObject = JObject.Parse(json);

                // "profiles" bölümü var mı kontrol et
                if (jsonObject.ContainsKey("profiles"))
                {
                    // Profilleri yükle
                    JObject profilesObject = (JObject)jsonObject["profiles"];
                    foreach (var property in profilesObject.Properties())
                    {
                        string profileName = property.Name;
                        JObject profileData = (JObject)property.Value;

                        Dictionary<string, CommandType> profile = new Dictionary<string, CommandType>();

                        // Bu profildeki buton eşlemelerini yükle
                        for (int i = 1; i <= 9; i++)
                        {
                            string buttonName = $"Button{i}";
                            if (profileData.ContainsKey(buttonName))
                            {
                                string enumValueStr = profileData[buttonName].ToString().Trim('"');
                                if (Enum.TryParse<CommandType>(enumValueStr, out var command))
                                {
                                    profile[buttonName] = command;
                                }
                                else
                                {
                                    profile[buttonName] = CommandType.None;
                                }
                            }
                            else
                            {
                                profile[buttonName] = CommandType.None;
                            }
                        }

                        profiles[profileName] = profile;
                    }

                    // Mevcut profili yükle
                    if (jsonObject.ContainsKey("currentProfile"))
                    {
                        currentProfileName = jsonObject["currentProfile"].ToString();
                    }
                    else
                    {
                        currentProfileName = profiles.Keys.FirstOrDefault() ?? "Default";
                    }

                    // Buton eşlemelerini mevcut profilden yükle
                    if (profiles.ContainsKey(currentProfileName))
                    {
                        buttonMappings = new Dictionary<string, CommandType>(profiles[currentProfileName]);
                    }
                    else
                    {
                        InitializeButtonMappings();
                    }
                }
                else
                {
                    // Eski format (tek profil) - "Default" profiline dönüştür
                    Dictionary<string, CommandType> defaultProfile = new Dictionary<string, CommandType>();

                    for (int i = 1; i <= 9; i++)
                    {
                        string buttonName = $"Button{i}";
                        if (jsonObject.ContainsKey(buttonName))
                        {
                            string enumValueStr = jsonObject[buttonName].ToString().Trim('"');
                            if (Enum.TryParse<CommandType>(enumValueStr, out var command))
                            {
                                defaultProfile[buttonName] = command;
                            }
                            else
                            {
                                defaultProfile[buttonName] = CommandType.None;
                            }
                        }
                        else
                        {
                            defaultProfile[buttonName] = CommandType.None;
                        }
                    }

                    profiles["Default"] = defaultProfile;
                    currentProfileName = "Default";
                    buttonMappings = new Dictionary<string, CommandType>(defaultProfile);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Config dosyası yüklenirken hata oluştu: {ex.Message}\nVarsayılan değerler kullanılacak.");
                InitializeDefaultProfiles();
                SaveButtonMappings();
            }
        }

        private void SaveButtonMappings()
        {
            try
            {
                // Mevcut ayarları profile kaydet
                SaveCurrentProfileSettings();

                // Tüm profilleri içeren JSON oluştur
                var jsonObject = new JObject();

                // Profiller bölümü
                var profilesObject = new JObject();
                foreach (var profilePair in profiles)
                {
                    var profileObject = new JObject();
                    foreach (var buttonPair in profilePair.Value)
                    {
                        profileObject[buttonPair.Key] = buttonPair.Value.ToString();
                    }
                    profilesObject[profilePair.Key] = profileObject;
                }

                jsonObject["profiles"] = profilesObject;
                jsonObject["currentProfile"] = currentProfileName;

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

            // Burada gelen komut tipini doğrudan belirle
            CommandType commandType;
            switch (rawCommandName)
            {
                case "VOLUME_DOWN": commandType = CommandType.VolumeDown; break;
                case "VOLUME_UP": commandType = CommandType.VolumeUp; break;
                case "BRIGHTNESS_DOWN": commandType = CommandType.BrightnessDown; break;
                case "BRIGHTNESS_UP": commandType = CommandType.BrightnessUp; break;
                case "OPEN_BROWSER": commandType = CommandType.OpenBrowser; break;
                case "CLOSE_APP": commandType = CommandType.CloseApp; break;
                case "LOCK_SCREEN": commandType = CommandType.LockScreen; break;
                case "ALT_TAB": commandType = CommandType.AltTab; break;
                case "CUSTOM_ACTION": commandType = CommandType.CustomAction; break;
                default:
                    Console.WriteLine($"❓ Tanınmayan komut: {rawCommandName}");
                    MessageBox.Show("Tanımlanmamış Komut: " + rawCommandName);
                    return;
            }

            // Komutu doğrudan çalıştır
            ExecuteCommand(commandType);
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
