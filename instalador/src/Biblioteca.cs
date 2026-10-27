// Biblioteca: a janela padrao do instalador. Acha sozinha os jogos das lojas (Lojas + Classificacao), mostra cada um
// como um cartao (Cartao.cs) com capa, grupo e estado do mod, e instala / atualiza / remove com o motor de sempre
// (Instalacao). Lojas, classificacao e capas rodam fora da thread da janela: a grade aparece com capas simples e as
// capas chegam depois, de UMA thread so (Capas.Obter nunca roda duas vezes ao mesmo tempo). Pasta sem escrita: a
// acao roda o proprio exe como administrador (--silencioso) e o texto e o codigo dele aparecem no log.
// A Janela de antes continua como a tela de "adicionar jogo manualmente".
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;

namespace Amdlss5.Instalador
{
    public sealed class Biblioteca : Form, IRelato
    {
        static readonly string[] Idiomas = { "en", "pt", "es" };
        static readonly string[] NomesIdiomas = { "English", "Português", "Español" };

        readonly string pastaPayload;
        readonly OpcoesLojas opLojas;
        readonly OpcoesCapas opCapas;
        readonly float escala;
        Payload payload;
        readonly List<IDisposable> recursos = new List<IDisposable>();
        Font fNome, fSelo, fCapa, fStatus, fPequena, fNegrito;

        // grade (so a thread da janela mexe)
        readonly List<Cartao> cartoes = new List<Cartao>();
        Cartao selecionado;
        int geracao;          // cada recarga da lista; resposta de uma recarga velha e descartada
        bool carregado;

        // capas: o dicionario e os contadores sao da thread da janela; a thread das capas so consome a fila
        readonly Dictionary<string, Image> capas = new Dictionary<string, Image>();
        readonly HashSet<string> capasPedidas = new HashSet<string>();
        int capasPendentes;
        readonly BlockingCollection<JogoLoja> filaCapas = new BlockingCollection<JogoLoja>();
        Thread threadCapas;

        bool ocupado;
        volatile bool mexendoNoJogo;   // instalar/remover em andamento: a janela nao fecha (a thread e de fundo)
        bool capturou;

        Label lblStatus, lblVersao, lblIdioma, lblEscolha, lblRodape;
        Label lblNome, lblGrupo, lblMotivo, lblEstado, lblExe, lblPasta, lblRe;
        ComboBox cmbIdioma;
        LinkLabel lnkManual;
        FlowLayoutPanel grade;
        Panel painel;
        CheckBox chkBaixar;
        Button btnInstalar, btnAtualizar, btnRemover;
        ProgressBar barra;
        TextBox txtLog;
        ToolTip dica;

        public string CapturaPng;      // --captura: espera a lista e as capas, abre o primeiro cartao, salva em PNG e fecha

        public Biblioteca(string pastaPayload, OpcoesLojas lojas, OpcoesCapas capas)
        {
            this.pastaPayload = pastaPayload;
            opLojas = lojas ?? new OpcoesLojas();
            opCapas = capas ?? new OpcoesCapas();
            using (Graphics g = Graphics.FromHwnd(IntPtr.Zero)) escala = Math.Max(1f, g.DpiX / 96f);
            try { Icon ico = Icon.ExtractAssociatedIcon(Application.ExecutablePath); recursos.Add(ico); Icon = ico; } catch (Exception) { }
            AutoScaleMode = AutoScaleMode.None;   // tudo em pixels via E(); as fontes (em pontos) ja seguem o dpi
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = Janela.Fundo;
            ForeColor = Janela.Texto;
            Font = Fonte("Segoe UI", 9.5F, FontStyle.Regular);
            ClientSize = new Size(E(1180), E(780));
            MinimumSize = new Size(E(960), E(620));
            Montar();
            Rectangle area = Screen.PrimaryScreen.WorkingArea;
            if (Width > area.Width || Height > area.Height) Size = new Size(Math.Min(Width, area.Width), Math.Min(Height, area.Height));
            try { payload = Payload.Carregar(pastaPayload); }
            catch (Exception e) { Linha(e.Message); }
            AplicarTextos();
        }

        int E(int v) { return (int)Math.Round(v * escala); }

        Font Fonte(string familia, float tamanho, FontStyle estilo)
        {
            Font f = new Font(familia, tamanho, estilo);
            recursos.Add(f);
            return f;
        }

        Label Rotulo(Control pai, Color cor, Font f)
        {
            Label l = new Label();
            l.AutoSize = false; l.ForeColor = cor; l.BackColor = Color.Transparent; l.Font = f; l.UseMnemonic = false;
            pai.Controls.Add(l);
            return l;
        }

        Button Botao(Color fundo)
        {
            Button b = new Button();
            b.FlatStyle = FlatStyle.Flat;
            b.FlatAppearance.BorderColor = Color.FromArgb(70, 70, 76);
            b.BackColor = fundo; b.ForeColor = Color.White; b.Font = fNegrito;
            b.Cursor = Cursors.Hand;
            // desligado com cara de desligado (o Flat so escurece o texto)
            b.EnabledChanged += delegate { b.BackColor = b.Enabled ? fundo : Color.FromArgb(38, 38, 42); b.FlatAppearance.BorderColor = Color.FromArgb(b.Enabled ? 70 : 52, b.Enabled ? 70 : 52, b.Enabled ? 76 : 58); };
            painel.Controls.Add(b);
            return b;
        }

        void Montar()
        {
            fNome = Fonte("Segoe UI", 9.5F, FontStyle.Bold);
            fSelo = Fonte("Segoe UI", 8F, FontStyle.Bold);
            fCapa = Fonte("Segoe UI", 13F, FontStyle.Bold);
            fStatus = Fonte("Segoe UI", 11F, FontStyle.Bold);
            fPequena = Fonte("Segoe UI", 8F, FontStyle.Regular);
            fNegrito = Fonte("Segoe UI", 10F, FontStyle.Bold);
            dica = new ToolTip(); recursos.Add(dica);
            int w = ClientSize.Width, h = ClientSize.Height, larguraPainel = E(380), topo = E(92);

            PictureBox logo = new PictureBox();
            logo.Bounds = new Rectangle(E(16), E(10), E(208), E(65));
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
            lblStatus = Rotulo(this, Janela.Texto, fStatus);
            lblStatus.Bounds = new Rectangle(E(240), E(18), w - E(240) - E(420), E(28));
            lblStatus.AutoEllipsis = true;
            lblVersao = Rotulo(this, Janela.Cinza, Fonte("Consolas", 9F, FontStyle.Regular));
            lblVersao.Bounds = new Rectangle(E(240), E(50), w - E(240) - E(420), E(22));

            cmbIdioma = new ComboBox();
            cmbIdioma.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbIdioma.FlatStyle = FlatStyle.Flat;
            cmbIdioma.BackColor = Janela.Painel; cmbIdioma.ForeColor = Janela.Texto;
            cmbIdioma.Items.AddRange(NomesIdiomas);
            cmbIdioma.DrawMode = DrawMode.OwnerDrawFixed;   // escuro tambem na caixa fechada
            cmbIdioma.ItemHeight = E(20);
            cmbIdioma.DrawItem += delegate (object o, DrawItemEventArgs e)
            {
                bool sel = (e.State & DrawItemState.Selected) != 0 && (e.State & DrawItemState.ComboBoxEdit) == 0;
                using (SolidBrush b = new SolidBrush(sel ? Color.FromArgb(60, 60, 68) : Janela.Painel)) e.Graphics.FillRectangle(b, e.Bounds);
                if (e.Index >= 0) TextRenderer.DrawText(e.Graphics, NomesIdiomas[e.Index], cmbIdioma.Font, e.Bounds, Janela.Texto, TextFormatFlags.VerticalCenter | TextFormatFlags.Left);
            };
            cmbIdioma.Bounds = new Rectangle(w - E(16) - E(130), E(14), E(130), E(26));
            cmbIdioma.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            cmbIdioma.SelectedIndex = Math.Max(0, Array.IndexOf(Idiomas, T.Codigo));
            cmbIdioma.SelectedIndexChanged += delegate { TrocarIdioma(); };
            Controls.Add(cmbIdioma);
            lblIdioma = Rotulo(this, Janela.Cinza, Font);
            lblIdioma.Bounds = new Rectangle(cmbIdioma.Left - E(6) - E(200), E(16), E(200), E(24));
            lblIdioma.TextAlign = ContentAlignment.MiddleRight;
            lblIdioma.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            lnkManual = new LinkLabel();
            lnkManual.AutoSize = false; lnkManual.TextAlign = ContentAlignment.MiddleRight; lnkManual.UseMnemonic = false;
            lnkManual.Bounds = new Rectangle(w - E(16) - E(400), E(52), E(400), E(24));
            lnkManual.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            lnkManual.LinkColor = Janela.Verde; lnkManual.ActiveLinkColor = Color.White; lnkManual.VisitedLinkColor = Janela.Verde;
            lnkManual.LinkClicked += delegate { AdicionarManual(); };
            Controls.Add(lnkManual);

            grade = new FlowLayoutPanel();
            grade.Bounds = new Rectangle(E(16), topo, w - E(16) - larguraPainel - E(28), h - topo - E(16));
            grade.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            grade.AutoScroll = true;
            grade.BackColor = Janela.Fundo;
            Controls.Add(grade);

            painel = new Panel();
            painel.Bounds = new Rectangle(w - E(16) - larguraPainel, topo, larguraPainel, h - topo - E(16));
            painel.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Right;
            painel.BackColor = Janela.Painel;
            Controls.Add(painel);
            lblEscolha = Rotulo(painel, Janela.Cinza, Font);
            lblNome = Rotulo(painel, Janela.Texto, Fonte("Segoe UI", 12F, FontStyle.Bold));
            lblGrupo = Rotulo(painel, Janela.Texto, fNegrito);
            lblMotivo = Rotulo(painel, Janela.Cinza, Font);
            lblEstado = Rotulo(painel, Janela.Texto, Font);
            lblExe = Rotulo(painel, Janela.Cinza, Font);
            lblPasta = Rotulo(painel, Janela.Cinza, Font);
            lblRe = Rotulo(painel, Janela.Cinza, Font);
            foreach (Label l in new[] { lblMotivo, lblPasta }) l.AutoEllipsis = true;
            chkBaixar = new CheckBox();
            chkBaixar.ForeColor = Janela.Cinza; chkBaixar.BackColor = Color.Transparent; chkBaixar.Checked = false;
            chkBaixar.TextAlign = ContentAlignment.TopLeft; chkBaixar.CheckAlign = ContentAlignment.TopLeft;
            painel.Controls.Add(chkBaixar);
            btnInstalar = Botao(Janela.VerdeSelo);
            btnAtualizar = Botao(Color.FromArgb(150, 105, 10));
            btnRemover = Botao(Color.FromArgb(150, 24, 30));
            barra = new ProgressBar();
            painel.Controls.Add(barra);
            txtLog = new TextBox();
            txtLog.Multiline = true; txtLog.ReadOnly = true; txtLog.ScrollBars = ScrollBars.Vertical; txtLog.WordWrap = true;
            txtLog.BackColor = Color.FromArgb(20, 20, 22); txtLog.ForeColor = Janela.Texto; txtLog.BorderStyle = BorderStyle.FixedSingle;
            txtLog.Font = Fonte("Consolas", 8.5F, FontStyle.Regular);
            painel.Controls.Add(txtLog);
            lblRodape = Rotulo(painel, Janela.Cinza, fPequena);
            painel.Resize += delegate { ArrumarPainel(); };

            btnInstalar.Click += delegate { InstalarClick(); };
            btnAtualizar.Click += delegate { InstalarClick(); };   // atualizar = instalar a versao do pacote por cima (o motor junta os backups)
            btnRemover.Click += delegate { RemoverClick(); };
        }

        // textos da janela no idioma atual (troca de idioma chama de novo)
        void AplicarTextos()
        {
            Text = T.S("bib_titulo");
            lblIdioma.Text = T.S("bib_idioma");
            lnkManual.Text = T.S("bib_manual");
            lblEscolha.Text = T.S("bib_escolha");
            btnInstalar.Text = T.S("btn_instalar");
            btnAtualizar.Text = T.S("bib_atualizar");
            btnRemover.Text = T.S("bib_remover");
            chkBaixar.Text = T.S("chk_baixar");
            lblVersao.Text = payload != null ? payload.Produto + " " + payload.Versao : "";
            lblRodape.Text = payload != null ? T.F("bib_rodape", payload.Versao) : T.S("sem_pacote");
            AtualizarStatus();
            foreach (Cartao c in cartoes) { c.Invalidate(); dica.SetToolTip(c, Dica(c.Dados)); }
            MostrarPainel();
        }

        void AtualizarStatus()
        {
            lblStatus.Text = !carregado ? T.S("bib_procurando") : cartoes.Count == 0 ? T.S("bib_nenhum") : T.F("bib_contagem", cartoes.Count);
        }

        void TrocarIdioma()
        {
            int i = cmbIdioma.SelectedIndex;
            if (i < 0 || Idiomas[i] == T.Codigo) return;
            T.Definir(Idiomas[i]);
            AplicarTextos();
            Recarregar();   // os motivos dos grupos sao traduzidos na classificacao
        }

        protected override void OnShown(EventArgs e)
        {
            base.OnShown(e);
            Recarregar();
            if (CapturaPng != null)
            {
                // reserva: sem lista ou capas em 25 s, salva o que tiver (o teste espera ate 30 s)
                System.Windows.Forms.Timer t = new System.Windows.Forms.Timer();
                t.Interval = 25000;
                t.Tick += delegate { t.Stop(); Capturar(); };
                t.Start();
            }
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (mexendoNoJogo && e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                MessageBox.Show(this, T.S("aguarde_fechar"), T.S("bib_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            base.OnFormClosing(e);
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing)
            {
                try { filaCapas.CompleteAdding(); } catch (Exception) { }
                foreach (Image im in capas.Values) { try { im.Dispose(); } catch (Exception) { } }
                capas.Clear();
                foreach (IDisposable d in recursos) { try { d.Dispose(); } catch (Exception) { } }
                recursos.Clear();
            }
        }

        // ------------------------------------------------------------------ IRelato (de qualquer thread)
        public void Linha(string texto)
        {
            if (InvokeRequired) { try { BeginInvoke(new Action<string>(Linha), texto); } catch (Exception) { } return; }
            txtLog.AppendText(texto + Environment.NewLine);
        }

        public void Progresso(int p)
        {
            if (InvokeRequired) { try { BeginInvoke(new Action<int>(Progresso), p); } catch (Exception) { } return; }
            barra.Value = Math.Max(0, Math.Min(100, p));
        }

        sealed class RelatoLista : IRelato
        {
            public readonly List<string> Linhas = new List<string>();
            public void Linha(string texto) { lock (Linhas) Linhas.Add(texto); }
            public void Progresso(int p) { }
        }

        // ------------------------------------------------------------------ lista de jogos
        void Recarregar()
        {
            int g = ++geracao;
            carregado = false;
            AtualizarStatus();
            Payload p = payload;
            Janela.EmSegundoPlano(this, delegate
            {
                RelatoLista r = new RelatoLista();
                List<Classificado> l = Classificacao.ListarClassificados(opLojas, p, r);
                l.Sort(delegate (Classificado a, Classificado b)
                {
                    int d = a.Grupo.CompareTo(b.Grupo);
                    return d != 0 ? d : string.Compare(a.Loja.Nome, b.Loja.Nome, StringComparison.CurrentCultureIgnoreCase);
                });
                return (object)new object[] { l, r.Linhas };
            }, delegate (object res, Exception e)
            {
                if (g != geracao) return;
                object[] par = res as object[];
                if (e != null) Linha(e.Message);
                else
                {
                    // as linhas de maquina (LOJA_ERRO/JOGO_ERRO, iguais as do --listar) viram texto traduzido, uma vez cada
                    HashSet<string> ditas = new HashSet<string>(StringComparer.Ordinal);
                    foreach (string linha in (List<string>)par[1]) { string t = LinhaParaJanela(linha); if (ditas.Add(t)) Linha(t); }
                }
                MostrarLista(par != null ? (List<Classificado>)par[0] : new List<Classificado>());
            });
        }

        // "LOJA_ERRO: steam: ..." -> "Could not read the Steam library."; "JOGO_ERRO: <pasta>: ..." -> "Could not check <pasta>."
        // (no idioma escolhido; o detalhe tecnico fica so no --listar). Outras linhas passam como estao.
        internal static string LinhaParaJanela(string linha)
        {
            if (linha == null) return "";
            const string loja = "LOJA_ERRO: ", jogo = "JOGO_ERRO: ";
            if (linha.StartsWith(loja, StringComparison.Ordinal))
            {
                string resto = linha.Substring(loja.Length);
                int dp = resto.IndexOf(':');
                return T.F("bib_erro_loja", NomeLoja(dp >= 0 ? resto.Substring(0, dp) : resto));
            }
            if (linha.StartsWith(jogo, StringComparison.Ordinal))
            {
                string resto = linha.Substring(jogo.Length);
                int dp = resto.IndexOf(": ", StringComparison.Ordinal);   // "D:\..." tem ":\", nunca ": " (':' nao existe em nome de pasta)
                string pasta = (dp >= 0 ? resto.Substring(0, dp) : resto).TrimEnd('\\', '/');
                string nome = Path.GetFileName(pasta);
                return T.F("bib_erro_jogo", string.IsNullOrEmpty(nome) ? pasta : nome);
            }
            return linha;
        }

        static string NomeLoja(string id)
        {
            switch ((id ?? "").Trim().ToLowerInvariant())
            {
                case "steam": return "Steam";
                case "epic": return "Epic Games";
                case "gog": return "GOG";
                case "ubisoft": return "Ubisoft Connect";
                case "ea": return "EA app";
                case "battlenet": return "Battle.net";
                case "xbox": return "Xbox";
                default: return id;
            }
        }

        void MostrarLista(List<Classificado> lista)
        {
            string manter = selecionado != null ? selecionado.Chave : null;
            selecionado = null;
            grade.SuspendLayout();
            foreach (Cartao c in cartoes) { dica.SetToolTip(c, null); grade.Controls.Remove(c); c.Dispose(); }   // as capas sao do dicionario, nao do cartao
            cartoes.Clear();
            foreach (Classificado c in lista)
            {
                Cartao k = new Cartao(c, payload, escala, fNome, fSelo, fCapa);
                k.Margin = new Padding(E(6));
                Image im;
                if (capas.TryGetValue(k.Chave, out im)) k.Capa = im;
                Cartao este = k;
                k.Click += delegate { Selecionar(este); };
                dica.SetToolTip(k, Dica(c));
                cartoes.Add(k);
                grade.Controls.Add(k);
                PedirCapa(c.Loja, k.Chave);
            }
            grade.ResumeLayout();
            carregado = true;
            AtualizarStatus();
            Selecionar(cartoes.Find(delegate (Cartao c) { return c.Chave == manter; }));
            CapturarSePronto();
        }

        static string Dica(Classificado c)
        {
            return c.Loja.Nome + "\r\n" + Classificacao.TextoGrupo(c.Grupo) + (c.Motivo.Length > 0 ? "\r\n" + c.Motivo : "");
        }

        // Classifica de novo um jogo (depois de instalar/remover) e atualiza o cartao dele.
        void Redetectar(Classificado antigo)
        {
            Payload p = payload;
            int g = geracao;
            Janela.EmSegundoPlano(this, delegate { return (object)Classificacao.Classificar(antigo.Loja, p); }, delegate (object res, Exception e)
            {
                if (e != null) { Linha(e.Message); return; }
                if (g != geracao) return;   // a lista inteira foi recarregada no meio
                Classificado novo = (Classificado)res;
                string chave = Cartao.ChaveDe(novo.Loja);
                foreach (Cartao c in cartoes)
                {
                    if (c.Chave != chave) continue;
                    c.Definir(novo, payload);
                    dica.SetToolTip(c, Dica(novo));
                    if (c == selecionado) MostrarPainel();
                }
            });
        }

        void Selecionar(Cartao c)
        {
            if (ocupado) return;
            if (selecionado != null) selecionado.Selecionado = false;
            selecionado = c;
            if (c != null) c.Selecionado = true;
            MostrarPainel();
        }

        // ------------------------------------------------------------------ capas
        void PedirCapa(JogoLoja l, string chave)
        {
            if (!capasPedidas.Add(chave)) return;   // ja pedida (ou ja chegou) numa recarga anterior
            capasPendentes++;
            if (threadCapas == null)
            {
                int w = E(Cartao.Largura), h = w * 3 / 2;
                threadCapas = new Thread(delegate () { FioCapas(w, h); });
                threadCapas.IsBackground = true;
                threadCapas.Start();
            }
            filaCapas.Add(l);
        }

        // a unica thread que chama Capas.Obter: um jogo por vez, na ordem da grade
        void FioCapas(int w, int h)
        {
            try
            {
                foreach (JogoLoja l in filaCapas.GetConsumingEnumerable())
                {
                    Image im = null;
                    try { string arq = Capas.Obter(l, opCapas); if (arq != null) im = Reduzir(arq, w, h); }
                    catch (Exception) { im = null; }
                    try { BeginInvoke(new Action<string, Image>(CapaChegou), Cartao.ChaveDe(l), im); }
                    catch (Exception) { if (im != null) im.Dispose(); return; }   // janela fechada
                }
            }
            catch (Exception) { }
        }

        // a imagem no tamanho da capa: vertical preenche (corta o que sobrar); horizontal (header da Steam) fica no meio
        static Image Reduzir(string arquivo, int w, int h)
        {
            using (MemoryStream ms = new MemoryStream(File.ReadAllBytes(arquivo)))
            using (Image src = Image.FromStream(ms))
            {
                Bitmap b = new Bitmap(w, h);
                using (Graphics g = Graphics.FromImage(b))
                {
                    g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                    g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                    g.Clear(Color.FromArgb(24, 24, 28));
                    float sx = (float)w / src.Width, sy = (float)h / src.Height;
                    float k = (float)src.Width / src.Height < 0.8f ? Math.Max(sx, sy) : sx;
                    float dw = src.Width * k, dh = src.Height * k;
                    g.DrawImage(src, (w - dw) / 2, (h - dh) / 2, dw, dh);
                }
                return b;
            }
        }

        void CapaChegou(string chave, Image im)
        {
            capasPendentes--;
            if (im != null)
            {
                capas[chave] = im;
                foreach (Cartao c in cartoes) if (c.Chave == chave) c.Capa = im;
            }
            CapturarSePronto();
        }

        void CapturarSePronto()
        {
            if (CapturaPng != null && carregado && capasPendentes == 0) Capturar();
        }

        void Capturar()
        {
            if (capturou) return;
            capturou = true;
            if (selecionado == null && cartoes.Count > 0) Selecionar(cartoes[0]);   // a captura mostra tambem o painel
            System.Windows.Forms.Timer t = new System.Windows.Forms.Timer();
            t.Interval = 400;
            t.Tick += delegate
            {
                t.Stop();
                try
                {
                    using (Bitmap b = new Bitmap(Width, Height))
                    {
                        DrawToBitmap(b, new Rectangle(0, 0, Width, Height));
                        b.Save(CapturaPng, System.Drawing.Imaging.ImageFormat.Png);
                    }
                }
                catch (Exception) { }
                Close();
            };
            t.Start();
        }

        // ------------------------------------------------------------------ painel do jogo escolhido
        void MostrarPainel()
        {
            Classificado c = selecionado != null ? selecionado.Dados : null;
            Jogo j = c != null ? c.Jogo : null;
            lblEscolha.Visible = c == null;
            foreach (Control k in new Control[] { lblNome, lblGrupo, lblMotivo, lblEstado, lblExe, lblPasta, lblRe, chkBaixar, btnInstalar, btnAtualizar, btnRemover })
                k.Visible = c != null;
            if (c != null)
            {
                EstadoMod est = Cartao.Estado(j, payload);
                bool pode = c.Grupo != Grupo.NaoSuportado;
                lblNome.Text = c.Loja.Nome;
                lblGrupo.Text = Classificacao.TextoGrupo(c.Grupo);
                lblGrupo.ForeColor = c.Grupo == Grupo.Testado ? Janela.Verde : c.Grupo == Grupo.NaoTestado ? Janela.Amarelo : Janela.Vermelho;
                lblMotivo.Text = c.Motivo;
                lblMotivo.Visible = c.Motivo.Length > 0;
                string estado = est == EstadoMod.Atualizar ? Cartao.TextoAtualizar(j, payload) : Cartao.TextoEstado(j, payload);
                if (est == EstadoMod.NaoInstalado && j.ProxyNossoSemBackup) estado += " - " + T.S("v_mod_sem_backup");
                lblEstado.Text = T.S("k_estado") + ": " + estado;
                lblEstado.ForeColor = est == EstadoMod.Instalado ? Janela.Verde : est == EstadoMod.Atualizar ? Janela.Amarelo : Janela.Texto;
                lblExe.Text = T.S("k_exe") + ": " + c.ExeNome;
                lblPasta.Text = T.S("k_pasta") + ": " + (j.PastaMod ?? c.Loja.Pasta) + (j.TemStorage ? T.S("v_storage") : "");
                lblRe.Text = j.ReEngine ? T.S("k_re") + ": " + T.F("v_re_sim", j.ReMotivo) : "";
                lblRe.Visible = j.ReEngine;
                // o download do REFramework so faz sentido se o pacote nao o trouxer (o de lancamento traz)
                chkBaixar.Visible = pode && j.ReEngine && payload != null && payload.ReframeworkEmbutido() == null;
                btnInstalar.Visible = pode;       // nao suportado: sem botao de instalar
                btnAtualizar.Visible = pode;
                btnInstalar.Enabled = !ocupado && payload != null && est == EstadoMod.NaoInstalado && j.Bloqueios.Count == 0;
                btnAtualizar.Enabled = !ocupado && payload != null && est == EstadoMod.Atualizar && j.Bloqueios.Count == 0;
                btnRemover.Enabled = !ocupado && j.SessoesBackup > 0;
                chkBaixar.Enabled = !ocupado;
            }
            cmbIdioma.Enabled = lnkManual.Enabled = !ocupado;
            ArrumarPainel();
        }

        // posicoes do painel pela altura real de cada texto (os 3 idiomas tem tamanhos diferentes)
        void ArrumarPainel()
        {
            int x = E(14), w = painel.ClientSize.Width - 2 * x, y = E(14);
            TextFormatFlags quebra = TextFormatFlags.WordBreak | TextFormatFlags.TextBoxControl;
            foreach (Label l in new[] { lblEscolha, lblNome, lblGrupo, lblMotivo, lblEstado, lblExe, lblPasta, lblRe })
            {
                if (!l.Visible) continue;
                int h = TextRenderer.MeasureText(l.Text.Length > 0 ? l.Text : "A", l.Font, new Size(w, int.MaxValue), quebra).Height + E(2);
                if (l == lblMotivo) h = Math.Min(h, E(110));
                if (l == lblPasta) h = Math.Min(h, E(44));
                l.Bounds = new Rectangle(x, y, w, h);
                y += h + E(l == lblNome ? 2 : 6);
            }
            if (chkBaixar.Visible)
            {
                int h = TextRenderer.MeasureText(chkBaixar.Text, chkBaixar.Font, new Size(w - E(24), int.MaxValue), quebra).Height + E(6);
                chkBaixar.Bounds = new Rectangle(x, y, w, h);
                y += h + E(6);
            }
            if (btnRemover.Visible)
            {
                y += E(4);
                List<Button> bs = new List<Button>();
                foreach (Button b in new[] { btnInstalar, btnAtualizar, btnRemover }) if (b.Visible) bs.Add(b);
                int bw = (w - E(8) * (bs.Count - 1)) / bs.Count;
                for (int i = 0; i < bs.Count; i++) bs[i].Bounds = new Rectangle(x + i * (bw + E(8)), y, bw, E(38));
                y += E(38) + E(10);
            }
            barra.Bounds = new Rectangle(x, y, w, E(8));
            y += E(8) + E(8);
            int hr = TextRenderer.MeasureText(lblRodape.Text, lblRodape.Font, new Size(w, int.MaxValue), quebra).Height + E(2);
            int yr = painel.ClientSize.Height - E(10) - hr;
            lblRodape.Bounds = new Rectangle(x, yr, w, hr);
            txtLog.Bounds = new Rectangle(x, y, w, Math.Max(E(60), yr - E(8) - y));
        }

        void Ocupado(bool sim) { ocupado = sim; UseWaitCursor = sim; MostrarPainel(); }

        void AdicionarManual()
        {
            if (ocupado) return;
            using (Janela jn = new Janela(pastaPayload)) jn.ShowDialog(this);
            Recarregar();   // pode ter instalado ou removido algo num jogo da lista
        }

        // ------------------------------------------------------------------ instalar / atualizar / remover
        void InstalarClick()
        {
            if (selecionado == null || payload == null || ocupado) return;
            Classificado c = selecionado.Dados;
            Jogo j = c.Jogo;
            if (c.Grupo == Grupo.NaoSuportado) return;
            if (j.Bloqueios.Count > 0)
            {
                MessageBox.Show(this, T.F("bloqueado", string.Join("\r\n", j.Bloqueios.ToArray())), T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            string avisos = string.Join("\r\n\r\n", j.Avisos.ToArray());
            if (c.Grupo == Grupo.NaoTestado)
            {
                if (MessageBox.Show(this, T.F("bib_confirma_nao_testado", c.Loja.Nome, avisos.Length > 0 ? "\r\n\r\n" + avisos : ""), T.S("avisos_titulo"),
                        MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2) != DialogResult.Yes) return;
            }
            else if (avisos.Length > 0 && MessageBox.Show(this, T.F("avisos_continuar", avisos), T.S("avisos_titulo"),
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2) != DialogResult.Yes) return;

            OpcoesInstalacao o = new OpcoesInstalacao();
            o.InstalarReframework = j.ReEngine;
            o.PermitirDownload = chkBaixar.Visible && chkBaixar.Checked;
            o.Idioma = T.Codigo;
            if (!Janela.PedirArquivosNvidia(this, payload, o, this)) return;
            Payload p = payload;
            Acao(c, o, delegate { return Instalacao.Instalar(j, p, o, this); });
        }

        void RemoverClick()
        {
            if (selecionado == null || ocupado) return;
            Classificado c = selecionado.Dados;
            Jogo j = c.Jogo;
            if (MessageBox.Show(this, T.F("bib_confirma_remover", j.PastaMod), T.S("bib_remover"), MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            Acao(c, null, delegate { return Instalacao.Desinstalar(j, this); });
        }

        sealed class SemEscrita { public string Mensagem; }

        // o = null: remover. Pasta sem escrita: nada e tentado aqui, a acao vai para o exe elevado.
        void Acao(Classificado c, OpcoesInstalacao o, Func<string> trabalho)
        {
            Jogo j = c.Jogo;
            txtLog.Clear();
            barra.Value = 0;
            mexendoNoJogo = true;
            Ocupado(true);
            Janela.EmSegundoPlano(this, delegate
            {
                try
                {
                    Util.TestarEscrita(j.PastaMod);
                    string storage = Path.Combine(j.PastaMod, "_storage_");
                    if (o != null && Directory.Exists(storage)) Util.TestarEscrita(storage);
                }
                catch (ErroInstalador e)
                {
                    if (e.Codigo != CodigoErro.Permissao) throw;
                    return new SemEscrita { Mensagem = e.Message };
                }
                return trabalho();
            }, delegate (object r, Exception e)
            {
                mexendoNoJogo = false;
                Ocupado(false);
                SemEscrita se = r as SemEscrita;
                ErroInstalador ei = e as ErroInstalador;
                if (se != null || (ei != null && ei.Codigo == CodigoErro.Permissao))
                {
                    string msg = se != null ? se.Mensagem : ei.Message;
                    Linha(msg);
                    if (MessageBox.Show(this, T.F("bib_admin", msg), T.S("permissao_titulo"), MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                    {
                        ComoAdministrador(c, o);
                        return;
                    }
                }
                else if (e != null) TratarErro(e);
                else Sucesso(j, o == null, r as string);
                Redetectar(c);
            });
        }

        void Sucesso(Jogo j, bool remover, string logDesinstalacao)
        {
            if (!remover) { MessageBox.Show(this, T.F("bib_sucesso", j.PastaMod), T.S("sucesso_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Information); return; }
            bool limpo = !Sessoes.Resta(j.PastaMod);
            MessageBox.Show(this, limpo ? T.S("des_sucesso") : T.F("des_parcial", Path.Combine(j.PastaMod, Util.PastaBackup), logDesinstalacao),
                T.S("des_titulo"), MessageBoxButtons.OK, limpo ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
        }

        void TratarErro(Exception e)
        {
            ErroInstalador ei = e as ErroInstalador;
            Linha(e.Message);
            if (ei != null && ei.Codigo == CodigoErro.JogoAberto)
                MessageBox.Show(this, e.Message, T.S("jogo_aberto_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else MessageBox.Show(this, e.Message, T.S("erro_titulo"), MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        // Roda o proprio exe elevado (--silencioso --instalar|--desinstalar <exe> --saida <tmp>) e mostra o texto e o
        // codigo dele no log. So esta acao pede administrador; a janela continua sem.
        void ComoAdministrador(Classificado c, OpcoesInstalacao o)
        {
            Jogo j = c.Jogo;
            string saida = Path.Combine(Util.TempInstalador(), "elevado-" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".txt");
            string args = "--silencioso " + (o == null ? "--desinstalar " : "--instalar ") + Util.Argumento(j.Exe) + " --payload " + Util.Argumento(pastaPayload)
                + " --idioma " + T.Codigo + " --saida " + Util.Argumento(saida);
            if (o != null)
            {
                args += " --reframework " + (o.InstalarReframework ? "sim" : "nao");
                if (o.PermitirDownload) args += " --permitir-download";
                if (j.Avisos.Count > 0) args += " --forcar";   // o usuario ja aceitou os avisos aqui na janela
                string f;
                if (o.Fornecidos.TryGetValue(Util.NomeNvngx, out f)) args += " --nvngx " + Util.Argumento(f);
                if (o.Fornecidos.TryGetValue(Util.NomePesos, out f)) args += " --pesos " + Util.Argumento(f);
            }
            Linha(T.S("bib_admin_rodando"));
            mexendoNoJogo = true;
            Ocupado(true);
            Janela.EmSegundoPlano(this, delegate
            {
                ProcessStartInfo psi = new ProcessStartInfo(Application.ExecutablePath, args);
                psi.Verb = "runas";
                psi.UseShellExecute = true;
                psi.WindowStyle = ProcessWindowStyle.Hidden;
                try
                {
                    using (Process pr = Process.Start(psi))
                    {
                        if (pr == null) throw new ErroInstalador(CodigoErro.Geral, T.F("bib_admin_falhou", "-"));
                        pr.WaitForExit();
                        return (object)new object[] { pr.ExitCode, File.Exists(saida) ? File.ReadAllText(saida) : "" };
                    }
                }
                finally { try { File.Delete(saida); } catch (Exception) { } }
            }, delegate (object r, Exception e)
            {
                mexendoNoJogo = false;
                Ocupado(false);
                if (e != null) Linha(e.Message);   // inclui o "nao" do usuario no pedido do Windows
                else
                {
                    object[] res = (object[])r;
                    int codigo = (int)res[0];
                    string erro = null;
                    foreach (string l in ((string)res[1]).Split(new[] { "\r\n", "\n" }, StringSplitOptions.RemoveEmptyEntries))
                    {
                        Linha(l);
                        if (l.StartsWith("ERRO: ")) erro = l.Substring(6);
                    }
                    Linha(T.F("bib_admin_codigo", codigo));
                    if (codigo == 0) Sucesso(j, o == null, "");
                    else MessageBox.Show(this, erro ?? T.F("bib_admin_falhou", codigo), T.S(codigo == (int)CodigoErro.JogoAberto ? "jogo_aberto_titulo" : "erro_titulo"),
                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                Redetectar(c);
            });
        }
    }
}
