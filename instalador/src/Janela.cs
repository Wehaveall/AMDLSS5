// Janela do instalador (WinForms, tema escuro do menu do mod). Todo o trabalho pesado (deteccao,
// hash, copia, download) roda fora da thread da janela; o andamento chega por IRelato.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace Amdlss5.Instalador
{
    public sealed class Janela : Form, IRelato
    {
        internal static readonly Color Fundo = Color.FromArgb(20, 20, 22);
        internal static readonly Color Painel = Color.FromArgb(13, 13, 14);
        internal static readonly Color Texto = Color.FromArgb(219, 224, 232);
        internal static readonly Color Cinza = Color.FromArgb(151, 161, 176);
        internal static readonly Color Verde = Color.FromArgb(118, 185, 0);
        internal static readonly Color VerdeSelo = Color.FromArgb(28, 132, 28);
        internal static readonly Color Vermelho = Color.FromArgb(237, 28, 36);
        internal static readonly Color Amarelo = Color.FromArgb(240, 190, 60);

        readonly string pastaPayload;
        Payload payload;
        string erroPayload;
        Jogo jogo;
        bool ocupado;
        volatile bool mexendoNoJogo;   // instalar/desinstalar em andamento: a janela nao fecha (a thread e de fundo e morreria no meio)
        readonly List<IDisposable> recursos = new List<IDisposable>();   // fontes, icone e logo criados aqui (liberados no Dispose)

        TextBox txtCaminho, txtLog;
        Button btnPasta, btnExe, btnInstalar, btnDesinstalar, btnAbrir, btnLogs;
        CheckBox chkRef, chkBaixar;
        ProgressBar barra;
        Label lblRodape;
        readonly Dictionary<string, Label> valores = new Dictionary<string, Label>();

        public string CapturaPng;      // --captura: salva a janela em PNG e fecha
        public string PastaInicial;    // --pasta: ja abre com o jogo escolhido

        public Janela(string pastaPayload)
        {
            this.pastaPayload = pastaPayload;
            Text = T.S("titulo");
            try { Icon ico = Icon.ExtractAssociatedIcon(Application.ExecutablePath); recursos.Add(ico); Icon = ico; } catch (Exception) { }
            AutoScaleMode = AutoScaleMode.Dpi;
            AutoScaleDimensions = new SizeF(96F, 96F);
            ClientSize = new Size(780, 780);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = Fundo;
            ForeColor = Texto;
            Font = Fonte("Segoe UI", 9.5F, FontStyle.Regular);
            Montar();
        }

        Font Fonte(string familia, float tamanho, FontStyle estilo)
        {
            Font f = new Font(familia, tamanho, estilo);
            recursos.Add(f);
            return f;
        }

        // Fechar no meio de instalar/desinstalar mataria a thread de fundo com a pasta do jogo pela metade (o desfazer
        // automatico so roda dentro dela). So o fechamento pedido pelo usuario e barrado; o do Windows (desligar) nao.
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (mexendoNoJogo && e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                MessageBox.Show(this, T.S("aguarde_fechar"), T.S("titulo"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            base.OnFormClosing(e);
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing)
            {
                foreach (IDisposable d in recursos) { try { d.Dispose(); } catch (Exception) { } }
                recursos.Clear();
            }
        }

        Label Rotulo(string texto, int x, int y, int w, Color cor, Font f)
        {
            Label l = new Label();
            l.Text = texto; l.Left = x; l.Top = y; l.Width = w; l.AutoSize = false; l.Height = (int)(f.GetHeight() * 1.45) + 2;
            l.ForeColor = cor; l.BackColor = Color.Transparent; l.Font = f; l.UseMnemonic = false;
            Controls.Add(l);
            return l;
        }

        Button Botao(string texto, int x, int y, int w, int h, Color fundo)
        {
            Button b = new Button();
            b.Text = texto; b.Left = x; b.Top = y; b.Width = w; b.Height = h;
            b.FlatStyle = FlatStyle.Flat;
            b.FlatAppearance.BorderColor = Color.FromArgb(70, 70, 76);
            b.BackColor = fundo; b.ForeColor = Color.White;
            b.Cursor = Cursors.Hand;
            Controls.Add(b);
            return b;
        }

        void Montar()
        {
            Font negrito = Fonte("Segoe UI", 10F, FontStyle.Bold);
            PictureBox logo = new PictureBox();
            logo.Left = 20; logo.Top = 16; logo.Width = 384; logo.Height = 120;
            logo.SizeMode = PictureBoxSizeMode.Zoom;
            logo.BackColor = Color.Transparent;
            Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream("Amdlss5.logo.png");
            if (s != null)
            {
                recursos.Add(s);   // Image.FromStream precisa do fluxo aberto enquanto a imagem viver
                Image img = Image.FromStream(s);
                recursos.Insert(0, img);
                logo.Image = img;
            }
            Controls.Add(logo);
            Label sub = Rotulo(T.S("subtitulo"), 424, 22, 340, Cinza, Font);
            sub.Height = 84;
            Label ver = Rotulo("", 424, 112, 340, Cinza, Fonte("Consolas", 9F, FontStyle.Regular));
            ver.Name = "versao";

            // 09/10 (usuario: botoes cortados): o titulo em negrito e mais alto que o espaco antigo e cobria o topo dos botoes.
            Label p1 = Rotulo(T.S("passo1"), 20, 148, 740, Texto, negrito);
            int yb = p1.Top + p1.Height + 6;
            btnPasta = Botao(T.S("btn_pasta"), 20, yb, 290, 38, Color.FromArgb(40, 40, 46));
            btnExe = Botao(T.S("btn_exe"), 320, yb, 220, 38, Color.FromArgb(40, 40, 46));
            txtCaminho = new TextBox();
            txtCaminho.Left = 20; txtCaminho.Top = yb + 46; txtCaminho.Width = 740; txtCaminho.TabStop = false; txtCaminho.ReadOnly = true;
            txtCaminho.BackColor = Painel; txtCaminho.ForeColor = Texto; txtCaminho.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(txtCaminho);

            Label p2 = Rotulo(T.S("passo2"), 20, txtCaminho.Top + 36, 740, Texto, negrito);
            string[] chaves = { "k_jogo", "k_exe", "k_pasta", "k_dx12", "k_fsr", "k_re", "k_nvidia", "k_estado" };
            int y = p2.Top + p2.Height + 4;
            // 09/10 (usuario: "linhas comidas"): o passo segue a altura real do rotulo (antes 25 px fixos, menor que o rotulo,
            // e o rotulo de baixo cobria o de cima). A janela cresce na vertical o quanto precisar.
            int passo = (int)(Font.GetHeight() * 1.45) + 6;
            foreach (string k in chaves)
            {
                Rotulo(T.S(k), 30, y, 175, Cinza, Font);
                Label v = Rotulo("", 210, y, 550, Texto, Font);
                v.AutoEllipsis = true;
                valores[k] = v;
                y += passo;
            }
            chkRef = new CheckBox();
            chkRef.Text = T.S("chk_ref"); chkRef.Left = 30; chkRef.Top = y + 4; chkRef.Width = 730; chkRef.Height = 28;
            chkRef.ForeColor = Texto; chkRef.BackColor = Color.Transparent;
            Controls.Add(chkRef);
            chkBaixar = new CheckBox();
            chkBaixar.Text = T.S("chk_baixar"); chkBaixar.Left = 50; chkBaixar.Top = y + 32; chkBaixar.Width = 710; chkBaixar.Height = 28;
            chkBaixar.ForeColor = Cinza; chkBaixar.BackColor = Color.Transparent; chkBaixar.Checked = false;
            Controls.Add(chkBaixar);
            y += 30;

            btnInstalar = Botao(T.S("btn_instalar"), 20, y + 38, 200, 40, VerdeSelo);
            btnInstalar.Font = negrito;
            btnDesinstalar = Botao(T.S("btn_desinstalar"), 230, y + 38, 160, 40, Color.FromArgb(150, 24, 30));
            btnAbrir = Botao(T.S("btn_abrir"), 400, y + 38, 140, 40, Color.FromArgb(40, 40, 46));
            btnLogs = Botao(T.S("btn_logs"), 550, y + 38, 210, 40, Color.FromArgb(40, 40, 46));

            barra = new ProgressBar();
            barra.Left = 20; barra.Top = y + 88; barra.Width = 740; barra.Height = 10;
            Controls.Add(barra);

            txtLog = new TextBox();
            int logTopo = y + 106, logAltura = 150, rodapeY = logTopo + logAltura + 8;
            txtLog.Left = 20; txtLog.Top = logTopo; txtLog.Width = 740; txtLog.Height = logAltura;
            txtLog.Multiline = true; txtLog.ReadOnly = true; txtLog.ScrollBars = ScrollBars.Vertical; txtLog.WordWrap = true;
            txtLog.BackColor = Painel; txtLog.ForeColor = Texto; txtLog.BorderStyle = BorderStyle.FixedSingle;
            txtLog.Font = Fonte("Consolas", 8.75F, FontStyle.Regular);
            Controls.Add(txtLog);
            lblRodape = Rotulo("", 20, rodapeY, 740, Cinza, Fonte("Segoe UI", 8F, FontStyle.Regular));
            ClientSize = new Size(780, rodapeY + lblRodape.Height + 12);

            btnPasta.Click += delegate { EscolherPasta(); };
            btnExe.Click += delegate { EscolherExe(); };
            btnInstalar.Click += delegate { InstalarClick(); };
            btnDesinstalar.Click += delegate { DesinstalarClick(); };
            btnLogs.Click += delegate { ColetarLogs(); };
            btnAbrir.Click += delegate
            {
                if (jogo != null && jogo.PastaMod != null) { ProcessStartInfo pasta = new ProcessStartInfo(jogo.PastaMod); pasta.UseShellExecute = true; Process.Start(pasta); }
            };

            try { payload = Payload.Carregar(pastaPayload); }
            catch (Exception e) { erroPayload = e.Message; }
            Controls.Find("versao", false)[0].Text = payload != null ? payload.Produto + " " + payload.Versao : "";
            lblRodape.Text = payload != null ? T.F("rodape", payload.Versao) : T.S("sem_pacote");
            if (erroPayload != null) Linha(erroPayload);
            MostrarJogo();
        }

        // 12/10: junta os logs e o INI do mod da pasta do jogo (e do _storage_) num zip na Area de Trabalho, para anexar a uma issue.
        void ColetarLogs()
        {
            if (jogo == null || jogo.PastaMod == null) return;
            try
            {
                List<string> arqs = new List<string>();
                foreach (string sub in new[] { "", "_storage_" })
                {
                    string d = Path.Combine(jogo.PastaMod, sub);
                    if (!Directory.Exists(d)) continue;
                    foreach (string pad in new[] { "dlssnr_on_amd.log", "dlssnr_on_amd.ini", "nr_*.log", "re2_framework_log.txt" })
                        foreach (string f in Directory.GetFiles(d, pad)) arqs.Add(f);
                }
                if (arqs.Count == 0) { MessageBox.Show(this, T.S("logs_nada"), T.S("btn_logs"), MessageBoxButtons.OK, MessageBoxIcon.Information); return; }
                string nome = "AMDLSS5-logs-" + Path.GetFileNameWithoutExtension(jogo.Exe) + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".zip";
                string zip = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), nome);
                using (ZipArchive z = ZipFile.Open(zip, ZipArchiveMode.Create))
                    foreach (string f in arqs)
                    {
                        string rel = f.Substring(jogo.PastaMod.Length).TrimStart('\\', '/').Replace('\\', '/');
                        using (FileStream src = new FileStream(f, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                        using (Stream dst = z.CreateEntry(rel).Open()) src.CopyTo(dst);
                    }
                Linha(T.F("logs_ok", zip));
                ProcessStartInfo ex = new ProcessStartInfo("explorer.exe", "/select,\"" + zip + "\""); ex.UseShellExecute = true; Process.Start(ex);
            }
            catch (Exception e) { MessageBox.Show(this, e.Message, T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }

        protected override void OnShown(EventArgs e)
        {
            base.OnShown(e);
            if (!string.IsNullOrEmpty(PastaInicial)) Detectar(PastaInicial);
            else if (CapturaPng != null) Capturar();
        }

        void Capturar()
        {
            System.Windows.Forms.Timer t = new System.Windows.Forms.Timer();
            t.Interval = 400;
            t.Tick += delegate
            {
                t.Stop();
                using (Bitmap b = new Bitmap(Width, Height))
                {
                    DrawToBitmap(b, new Rectangle(0, 0, Width, Height));
                    b.Save(CapturaPng, System.Drawing.Imaging.ImageFormat.Png);
                }
                Close();
            };
            t.Start();
        }

        // ------------------------------------------------------------------ IRelato (de qualquer thread)
        public void Linha(string texto)
        {
            if (InvokeRequired) { BeginInvoke(new Action<string>(Linha), texto); return; }
            txtLog.AppendText(texto + Environment.NewLine);
        }

        public void Progresso(int p)
        {
            if (InvokeRequired) { BeginInvoke(new Action<int>(Progresso), p); return; }
            barra.Value = Math.Max(0, Math.Min(100, p));
        }

        // ------------------------------------------------------------------ escolha do jogo
        void EscolherPasta()
        {
            string pasta = SeletorPasta.Escolher(Handle, T.S("dlg_pasta"));
            if (pasta != null) Detectar(pasta);
        }

        void EscolherExe()
        {
            using (OpenFileDialog d = new OpenFileDialog())
            {
                d.Title = T.S("dlg_exe");
                d.Filter = T.S("filtro_exe");
                d.CheckFileExists = true;
                if (d.ShowDialog(this) == DialogResult.OK) Detectar(d.FileName);
            }
        }

        void Detectar(string caminho)
        {
            txtCaminho.Text = caminho;
            txtCaminho.SelectionStart = txtCaminho.Text.Length; txtCaminho.SelectionLength = 0;
            ActiveControl = btnInstalar;
            Ocupado(true);
            valores["k_jogo"].Text = T.S("detectando");
            Payload p = payload;
            EmSegundoPlano(delegate { return (object)Detector.Detectar(caminho, p); }, delegate (object r, Exception e)
            {
                Ocupado(false);
                if (e != null) { Linha(e.Message); jogo = null; }
                else jogo = (Jogo)r;
                MostrarJogo();
                if (jogo != null)
                {
                    foreach (string b in jogo.Bloqueios) Linha("[X] " + b);
                    foreach (string a in jogo.Avisos) Linha("[!] " + a);
                    if (jogo.Exe != null) Linha(jogo.Nome + "  -  " + jogo.Exe);
                }
                if (CapturaPng != null) Capturar();
            });
        }

        void Valor(string k, string texto, Color cor) { valores[k].Text = texto; valores[k].ForeColor = cor; }

        void MostrarJogo()
        {
            if (jogo == null)
            {
                Valor("k_jogo", T.S("nenhum_jogo"), Cinza);
                foreach (string k in new[] { "k_exe", "k_pasta", "k_dx12", "k_fsr", "k_re", "k_nvidia", "k_estado" }) Valor(k, "", Texto);
                AtualizarBotoes();
                return;
            }
            Valor("k_jogo", jogo.Nome, Texto);
            if (jogo.Exe == null) Valor("k_exe", jogo.Bloqueios.Count > 0 ? jogo.Bloqueios[0] : "?", Vermelho);
            else Valor("k_exe", Path.GetFileName(jogo.Exe) + (jogo.Bloqueios.Count > 0 ? "  -  " + jogo.Bloqueios[0] : ""), jogo.Bloqueios.Count > 0 ? Vermelho : Verde);
            Valor("k_pasta", jogo.PastaMod == null ? "" : jogo.PastaMod + (jogo.TemStorage ? T.S("v_storage") : ""), Texto);
            if (jogo.Exe != null)
            {
                Valor("k_dx12", jogo.Dx12 ? T.S("v_sim") : T.S("v_nao_confirmado"), jogo.Dx12 ? Verde : Amarelo);
                if (jogo.FfxTipo == "monolitico" || jogo.FfxTipo == "split")
                    Valor("k_fsr", T.F("v_fsr_mono", Path.GetFileName(jogo.FfxArquivo), jogo.FfxVersao ?? "?"), Verde);
                else if (jogo.FfxTipo == "outro") Valor("k_fsr", T.F("v_fsr_outro", Path.GetFileName(jogo.FfxArquivo)), Amarelo);
                else Valor("k_fsr", T.S("v_fsr_nenhum"), Amarelo);
                Valor("k_re", jogo.ReEngine ? T.F("v_re_sim", jogo.ReMotivo) : T.S("v_re_nao"), Texto);
                if (payload != null)
                {
                    List<string> f = payload.FaltandoNvidia();
                    Valor("k_nvidia", f.Count == 0 ? T.S("v_nvidia_pacote") : T.F("v_nvidia_falta", string.Join(", ", f.ToArray())), f.Count == 0 ? Verde : Amarelo);
                }
                if (jogo.SessoesBackup > 0) Valor("k_estado", T.F("v_mod_sim", jogo.SessoesBackup), Verde);
                else if (jogo.ProxyNossoSemBackup) Valor("k_estado", T.S("v_mod_sem_backup"), Amarelo);
                else Valor("k_estado", T.S("v_mod_nao"), Texto);
                chkRef.Checked = jogo.ReEngine;
                chkRef.CheckedChanged -= RefMudou; chkRef.CheckedChanged += RefMudou;
            }
            AtualizarBotoes();
        }

        void AtualizarBotoes()
        {
            bool temJogo = jogo != null && jogo.Exe != null;
            btnPasta.Enabled = btnExe.Enabled = !ocupado;
            btnInstalar.Enabled = !ocupado && temJogo && payload != null && jogo.Bloqueios.Count == 0;
            btnDesinstalar.Enabled = !ocupado && temJogo && jogo.SessoesBackup > 0;
            btnAbrir.Enabled = !ocupado && temJogo;
            btnLogs.Enabled = !ocupado && temJogo;
            chkRef.Enabled = !ocupado && temJogo;
            // o download so faz sentido se o pacote nao trouxer o REFramework (o pacote de lancamento traz)
            chkBaixar.Enabled = !ocupado && temJogo && chkRef.Checked && payload != null && payload.ReframeworkEmbutido() == null;
        }

        void RefMudou(object o, EventArgs e) { AtualizarBotoes(); }

        void Ocupado(bool sim) { ocupado = sim; UseWaitCursor = sim; AtualizarBotoes(); }

        // ------------------------------------------------------------------ instalar / desinstalar
        void InstalarClick()
        {
            if (jogo == null || payload == null) return;
            if (jogo.Bloqueios.Count > 0)
            {
                MessageBox.Show(this, T.F("bloqueado", string.Join("\r\n", jogo.Bloqueios.ToArray())), T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            // anticheat (mesma regra da biblioteca): a pasta escolhida e a do exe; o modo silencioso nao muda
            if (Classificacao.TemAnticheat(jogo.PastaMod, jogo.Raiz))
            {
                Linha(T.S("motivo_anticheat"));
                MessageBox.Show(this, T.F("bloqueado", T.S("motivo_anticheat")), T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            if (jogo.Avisos.Count > 0 && MessageBox.Show(this, T.F("avisos_continuar", string.Join("\r\n\r\n", jogo.Avisos.ToArray())), T.S("avisos_titulo"),
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2) != DialogResult.Yes) return;

            OpcoesInstalacao o = new OpcoesInstalacao();
            o.InstalarReframework = chkRef.Checked;
            o.PermitirDownload = chkBaixar.Checked;
            o.Idioma = T.Codigo;
            if (!PedirArquivosNvidia(this, payload, o, this)) return;
            Jogo j = jogo;
            Payload p = payload;
            txtLog.Clear();
            Ocupado(true);
            mexendoNoJogo = true;
            EmSegundoPlano(delegate { return (object)Instalacao.Instalar(j, p, o, this); }, delegate (object r, Exception e)
            {
                mexendoNoJogo = false;
                Ocupado(false);
                if (e == null) MessageBox.Show(this, T.F("sucesso", j.PastaMod), T.S("sucesso_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                else TratarErro(e);
                Detectar(j.Escolhido);
            });
        }

        // Arquivos da NVIDIA que o pacote nao traz: pede cada um ao usuario (vao em o.Fornecidos). Falso = cancelado ou
        // arquivo invalido (nada a instalar). Tambem usado pela Biblioteca.
        internal static bool PedirArquivosNvidia(IWin32Window dono, Payload p, OpcoesInstalacao o, IRelato r)
        {
            foreach (string falta in p.FaltandoNvidia())
            {
                bool nv = falta == Util.NomeNvngx;
                MessageBox.Show(dono, T.S(nv ? "pedir_nvngx" : "pedir_pesos"), T.S("avisos_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                using (OpenFileDialog d = new OpenFileDialog())
                {
                    d.Title = falta;
                    d.Filter = T.S(nv ? "filtro_nvngx" : "filtro_pesos");
                    d.CheckFileExists = true;
                    if (d.ShowDialog(dono) != DialogResult.OK) { r.Linha(T.S("cancelado")); return false; }
                    if (nv)
                    {
                        InfoPe pe = Pe.Ler(d.FileName);
                        if (pe == null || !pe.X64 || !pe.EhDll)
                        {
                            MessageBox.Show(dono, T.F("nvngx_invalido", d.FileName), T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                            return false;
                        }
                    }
                    o.Fornecidos[falta] = d.FileName;
                }
            }
            return true;
        }

        void DesinstalarClick()
        {
            if (jogo == null) return;
            if (MessageBox.Show(this, T.F("confirma_des", jogo.PastaMod), T.S("btn_desinstalar"), MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            Jogo j = jogo;
            txtLog.Clear();
            Ocupado(true);
            mexendoNoJogo = true;
            EmSegundoPlano(delegate { return (object)Instalacao.Desinstalar(j, this); }, delegate (object r, Exception e)
            {
                mexendoNoJogo = false;
                Ocupado(false);
                if (e == null)
                {
                    bool limpo = !Sessoes.Resta(j.PastaMod);
                    MessageBox.Show(this, limpo ? T.S("des_sucesso") : T.F("des_parcial", Path.Combine(j.PastaMod, Util.PastaBackup), r),
                        T.S("des_titulo"), MessageBoxButtons.OK, limpo ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
                }
                else TratarErro(e);
                Detectar(j.Escolhido);
            });
        }

        void TratarErro(Exception e)
        {
            ErroInstalador ei = e as ErroInstalador;
            Linha(e.Message);
            if (ei != null && ei.Codigo == CodigoErro.JogoAberto)
                MessageBox.Show(this, e.Message, T.S("jogo_aberto_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else if (ei != null && ei.Codigo == CodigoErro.Permissao)
            {
                if (MessageBox.Show(this, T.F("permissao_admin", e.Message), T.S("permissao_titulo"), MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    try
                    {
                        // aspas pelas regras do Windows: "E:\" (raiz de um disco) nao pode virar E:" e engolir o --payload
                        ProcessStartInfo psi = new ProcessStartInfo(Application.ExecutablePath, "--pasta " + Util.Argumento(jogo.Escolhido) + " --payload " + Util.Argumento(pastaPayload) + " --idioma " + T.Codigo);
                        psi.Verb = "runas";
                        psi.UseShellExecute = true;
                        Process.Start(psi);
                        Close();
                    }
                    catch (Exception) { }
                }
            }
            else MessageBox.Show(this, e.Message, T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        void EmSegundoPlano(Func<object> trabalho, Action<object, Exception> fim) { EmSegundoPlano(this, trabalho, fim); }

        // trabalho numa thread de fundo; fim(resultado, erro) volta na thread da janela (tambem usado pela Biblioteca)
        internal static void EmSegundoPlano(Control dono, Func<object> trabalho, Action<object, Exception> fim)
        {
            Thread t = new Thread(delegate ()
            {
                object r = null;
                Exception erro = null;
                try { r = trabalho(); }
                catch (Exception e) { erro = e; }
                try { dono.BeginInvoke(fim, r, erro); } catch (Exception) { }
            });
            t.IsBackground = true;
            t.Start();
        }
    }

    // Seletor de pasta moderno do Windows (IFileOpenDialog com FOS_PICKFOLDERS); se falhar, cai no
    // FolderBrowserDialog classico.
    static class SeletorPasta
    {
        [ComImport, Guid("DC1C5A9C-E88A-4dde-A5A1-60F82A20AEF7")]
        class FileOpenDialogRCW { }

        [ComImport, Guid("42f85136-db7e-439c-85f1-e4075d135fc8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IFileDialog
        {
            [PreserveSig] int Show(IntPtr parent);
            void SetFileTypes(uint c, IntPtr specs);
            void SetFileTypeIndex(uint i);
            void GetFileTypeIndex(out uint i);
            void Advise(IntPtr sink, out uint cookie);
            void Unadvise(uint cookie);
            void SetOptions(uint fos);
            void GetOptions(out uint fos);
            void SetDefaultFolder(IShellItem item);
            void SetFolder(IShellItem item);
            void GetFolder(out IShellItem item);
            void GetCurrentSelection(out IShellItem item);
            void SetFileName([MarshalAs(UnmanagedType.LPWStr)] string nome);
            void GetFileName([MarshalAs(UnmanagedType.LPWStr)] out string nome);
            void SetTitle([MarshalAs(UnmanagedType.LPWStr)] string titulo);
            void SetOkButtonLabel([MarshalAs(UnmanagedType.LPWStr)] string texto);
            void SetFileNameLabel([MarshalAs(UnmanagedType.LPWStr)] string texto);
            void GetResult(out IShellItem item);
        }

        [ComImport, Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IShellItem
        {
            void BindToHandler(IntPtr pbc, ref Guid bhid, ref Guid riid, out IntPtr ppv);
            void GetParent(out IShellItem item);
            void GetDisplayName(uint sigdn, [MarshalAs(UnmanagedType.LPWStr)] out string nome);
            void GetAttributes(uint mask, out uint attribs);
            void Compare(IShellItem item, uint hint, out int ordem);
        }

        public static string Escolher(IntPtr dono, string titulo)
        {
            try
            {
                IFileDialog d = (IFileDialog)new FileOpenDialogRCW();
                uint op;
                d.GetOptions(out op);
                d.SetOptions(op | 0x20 | 0x40 | 0x800); // PICKFOLDERS | FORCEFILESYSTEM | PATHMUSTEXIST
                d.SetTitle(titulo);
                if (d.Show(dono) != 0) return null;
                IShellItem item;
                d.GetResult(out item);
                string caminho;
                item.GetDisplayName(0x80058000, out caminho); // SIGDN_FILESYSPATH
                return caminho;
            }
            catch (Exception)
            {
                using (FolderBrowserDialog f = new FolderBrowserDialog())
                {
                    f.Description = titulo;
                    return f.ShowDialog() == DialogResult.OK ? f.SelectedPath : null;
                }
            }
        }
    }
}
