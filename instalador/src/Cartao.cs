// Cartao de um jogo na biblioteca: capa 2:3 (a 600x900 da Steam reduzida, ou a capa simples desenhada com o nome),
// selo do grupo sobre a capa, nome e selo do estado do mod embaixo. Desenhado a mao; os textos vem de T na hora de
// pintar (trocar o idioma = Invalidate). As fontes sao da Biblioteca (ela as libera).
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace Amdlss5.Instalador
{
    public enum EstadoMod { NaoInstalado, Instalado, Atualizar }

    public sealed class Cartao : Control
    {
        public const int Largura = 168;                  // a 96 dpi; a capa tem Largura x Largura*3/2
        static readonly Color Fundo = Color.FromArgb(30, 30, 34);
        static readonly Color Borda = Color.FromArgb(52, 52, 58);

        Classificado dados;
        Payload payload;
        Image capa;
        bool selecionado, sobre;
        readonly float escala;
        readonly Font fonteNome, fonteSelo, fonteCapa;

        public Cartao(Classificado c, Payload p, float escala, Font nome, Font selo, Font capaSimples)
        {
            this.escala = escala;
            fonteNome = nome; fonteSelo = selo; fonteCapa = capaSimples;
            SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw | ControlStyles.Selectable, true);
            Cursor = Cursors.Hand;
            TabStop = true;
            AccessibleRole = AccessibleRole.PushButton;
            int w = E(Largura);
            Size = new Size(w, AlturaCapa(w) + E(6) + AlturaNome() + E(4) + AlturaSelo() + E(8));
            Definir(c, p);
        }

        int E(int v) { return (int)Math.Round(v * escala); }
        static int AlturaCapa(int w) { return w * 3 / 2; }
        int AlturaNome() { return TextRenderer.MeasureText("Ag", fonteNome).Height * 2; }
        int AlturaSelo() { return TextRenderer.MeasureText("Ag", fonteSelo).Height + E(6); }

        public Classificado Dados { get { return dados; } }
        public string Chave { get { return ChaveDe(dados.Loja); } }

        public void Definir(Classificado c, Payload p)
        {
            dados = c; payload = p;
            AccessibleName = Text = c.Loja.Nome;   // Text = nome da janela do controle (leitores de tela, UI Automation)
            Invalidate();
        }

        // a imagem ja vem no tamanho da capa (a Biblioteca e dona dela)
        public Image Capa { get { return capa; } set { capa = value; Invalidate(); } }
        public bool Selecionado { get { return selecionado; } set { if (selecionado != value) { selecionado = value; Invalidate(); } } }

        public static string ChaveDe(JogoLoja l) { return (l.Loja + "|" + l.Id + "|" + l.Pasta).ToLowerInvariant(); }

        public static EstadoMod Estado(Jogo j, Payload p)
        {
            if (j == null || j.VersaoInstalada == null) return EstadoMod.NaoInstalado;
            if (p != null && PacoteMaisNovo(j.VersaoInstalada, p.Versao)) return EstadoMod.Atualizar;
            return EstadoMod.Instalado;
        }

        // "Atualizar" so quando o pacote e MAIS NOVO que o instalado (nunca volta para uma versao velha). Compara os numeros
        // do comeco ("1.6.0" de "1.6.0-teste") com System.Version; numeros iguais: atualiza so se o instalado tem sufixo
        // (versao de teste -> a final). "?" (instalado por um instalador antigo, sem versao.txt) = atualizar. Versao sem
        // numeros no comeco (pacotes de teste: "teste") nao da para ordenar: atualiza se for diferente.
        public static bool PacoteMaisNovo(string instalada, string pacote)
        {
            if (instalada == null || pacote == null) return false;
            if (instalada == "?") return true;
            if (string.Equals(instalada, pacote, StringComparison.Ordinal)) return false;
            string sufI, sufP;
            Version vi = Numeros(instalada, out sufI), vp = Numeros(pacote, out sufP);
            if (vi == null || vp == null) return true;
            int c = vp.CompareTo(vi);
            if (c != 0) return c > 0;
            return sufI.Length > 0;
        }

        // "1.6.0-teste" -> 1.6.0.0 e "-teste"; sem numero no comeco = null
        static Version Numeros(string v, out string sufixo)
        {
            System.Text.RegularExpressions.Match m = System.Text.RegularExpressions.Regex.Match(v.Trim(), @"^v?(\d+(?:\.\d+){0,3})(.*)$", System.Text.RegularExpressions.RegexOptions.IgnoreCase);
            sufixo = m.Success ? m.Groups[2].Value : "";
            if (!m.Success) return null;
            string[] partes = m.Groups[1].Value.Split('.');
            int[] n = new int[4];
            for (int i = 0; i < partes.Length; i++) if (!int.TryParse(partes[i], out n[i])) return null;
            return new Version(n[0], n[1], n[2], n[3]);
        }

        public static string TextoEstado(Jogo j, Payload p)
        {
            EstadoMod e = Estado(j, p);
            if (e == EstadoMod.NaoInstalado) return T.S("bib_nao_instalado");
            if (e == EstadoMod.Atualizar) return T.S("bib_atualizar_selo");
            return j.VersaoInstalada == "?" ? T.S("bib_instalado") : T.F("bib_instalado_v", j.VersaoInstalada);
        }

        // linha do painel para "Atualizar": "Instalado: v1.5.2. O pacote traz a v1.6.0..."; "?" = versao desconhecida (nunca "v?")
        public static string TextoAtualizar(Jogo j, Payload p)
        {
            return j.VersaoInstalada == "?" ? T.F("bib_estado_atualizar_desconhecida", p.Versao) : T.F("bib_estado_atualizar", j.VersaoInstalada, p.Versao);
        }

        public static Color CorGrupo(Grupo g)
        {
            return g == Grupo.Testado ? Color.FromArgb(28, 132, 28) : g == Grupo.NaoTestado ? Color.FromArgb(166, 112, 8) : Color.FromArgb(140, 34, 40);
        }

        protected override void OnMouseEnter(EventArgs e) { sobre = true; Invalidate(); base.OnMouseEnter(e); }
        protected override void OnMouseLeave(EventArgs e) { sobre = false; Invalidate(); base.OnMouseLeave(e); }
        protected override void OnGotFocus(EventArgs e) { Invalidate(); base.OnGotFocus(e); }
        protected override void OnLostFocus(EventArgs e) { Invalidate(); base.OnLostFocus(e); }
        protected override void OnMouseDown(MouseEventArgs e) { Focus(); base.OnMouseDown(e); }
        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter || e.KeyCode == Keys.Space) { OnClick(EventArgs.Empty); e.Handled = true; }
            base.OnKeyDown(e);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.Clear(Parent != null ? Parent.BackColor : Color.Black);
            int w = Width, hc = AlturaCapa(w);
            using (GraphicsPath fundo = Arredondado(new Rectangle(0, 0, w - 1, Height - 1), E(6)))
            using (SolidBrush b = new SolidBrush(Fundo)) g.FillPath(b, fundo);

            Rectangle rc = new Rectangle(0, 0, w, hc);
            using (GraphicsPath corte = Arredondado(new Rectangle(0, 0, w - 1, hc + E(8)), E(6)))
            {
                Region antes = g.Clip;
                g.SetClip(corte, CombineMode.Intersect);
                g.SetClip(rc, CombineMode.Intersect);
                if (capa != null) g.DrawImage(capa, rc);
                else CapaSimples(g, rc, dados.Loja.Nome, fonteCapa, escala);
                if (dados.Grupo == Grupo.NaoSuportado)
                    using (SolidBrush escuro = new SolidBrush(Color.FromArgb(120, 0, 0, 0))) g.FillRectangle(escuro, rc);
                if (dados.Anticheat) Tarja(g, rc, T.S("bib_tarja_anticheat"));
                g.Clip = antes;
                antes.Dispose();
            }
            Selo(g, Classificacao.TextoGrupo(dados.Grupo), E(6), E(6), w - E(12), CorGrupo(dados.Grupo), Color.White);

            int y = hc + E(6);
            Rectangle rn = new Rectangle(E(8), y, w - E(16), AlturaNome());
            TextRenderer.DrawText(g, dados.Loja.Nome, fonteNome, rn, Color.FromArgb(219, 224, 232), TextFormatFlags.WordBreak | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix | TextFormatFlags.TextBoxControl);
            y += rn.Height + E(4);
            EstadoMod est = Estado(dados.Jogo, payload);
            Color fs = est == EstadoMod.Instalado ? Color.FromArgb(22, 72, 22) : est == EstadoMod.Atualizar ? Color.FromArgb(110, 80, 10) : Color.FromArgb(52, 52, 60);
            Color ts = est == EstadoMod.Instalado ? Color.FromArgb(150, 220, 90) : est == EstadoMod.Atualizar ? Color.FromArgb(250, 210, 100) : Color.FromArgb(170, 178, 190);
            Selo(g, TextoEstado(dados.Jogo, payload), E(8), y, w - E(16), fs, ts);

            Color borda = selecionado ? Color.FromArgb(118, 185, 0) : (sobre || Focused) ? Color.FromArgb(120, 120, 130) : Borda;
            using (GraphicsPath contorno = Arredondado(new Rectangle(0, 0, w - 1, Height - 1), E(6)))
            using (Pen p = new Pen(borda, selecionado ? E(2) : 1)) g.DrawPath(p, contorno);
        }

        // faixa vermelha de lado a lado no meio da capa (jogo com anticheat: nao instalar)
        void Tarja(Graphics g, Rectangle rc, string texto)
        {
            int h = Math.Max(E(44), rc.Height / 6);
            Rectangle r = new Rectangle(rc.X, rc.Y + (rc.Height - h) / 2, rc.Width, h);
            using (SolidBrush b = new SolidBrush(Color.FromArgb(235, 200, 20, 30))) g.FillRectangle(b, r);
            using (Pen p = new Pen(Color.FromArgb(255, 255, 210, 210), Math.Max(1, E(1))))
            { g.DrawLine(p, r.Left, r.Top, r.Right, r.Top); g.DrawLine(p, r.Left, r.Bottom - 1, r.Right, r.Bottom - 1); }
            TextRenderer.DrawText(g, texto, fonteSelo, Rectangle.Inflate(r, -E(6), 0), Color.White,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.WordBreak | TextFormatFlags.NoPrefix);
        }

        // pilula com o texto (cortado com ... se nao couber em maxW)
        void Selo(Graphics g, string texto, int x, int y, int maxW, Color fundo, Color cor)
        {
            Size t = TextRenderer.MeasureText(g, texto, fonteSelo, new Size(int.MaxValue, int.MaxValue), TextFormatFlags.NoPadding | TextFormatFlags.NoPrefix);
            int h = AlturaSelo(), w = Math.Min(maxW, t.Width + E(14));
            Rectangle r = new Rectangle(x, y, w, h);
            using (GraphicsPath p = Arredondado(r, h / 2))
            using (SolidBrush b = new SolidBrush(fundo)) g.FillPath(b, p);
            TextRenderer.DrawText(g, texto, fonteSelo, r, cor, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.SingleLine | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix | TextFormatFlags.NoPadding);
        }

        // capa sem imagem: degrade com a cor tirada do nome e o nome no meio
        public static void CapaSimples(Graphics g, Rectangle r, string nome, Font f, float escala)
        {
            int h = 0;
            foreach (char c in nome ?? "") h = h * 31 + c;
            float matiz = (h & 0x7fffffff) % 360;
            using (LinearGradientBrush b = new LinearGradientBrush(r, Hsv(matiz, 0.55f, 0.42f), Hsv((matiz + 40) % 360, 0.65f, 0.14f), LinearGradientMode.ForwardDiagonal))
                g.FillRectangle(b, r);
            int m = (int)Math.Round(12 * escala);
            Rectangle rt = Rectangle.Inflate(r, -m, -m);
            TextRenderer.DrawText(g, nome ?? "", f, rt, Color.White, TextFormatFlags.WordBreak | TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix);
        }

        static Color Hsv(float h, float s, float v)
        {
            float c = v * s, x = c * (1 - Math.Abs((h / 60f) % 2 - 1)), m = v - c;
            float r = 0, g = 0, b = 0;
            if (h < 60) { r = c; g = x; } else if (h < 120) { r = x; g = c; } else if (h < 180) { g = c; b = x; }
            else if (h < 240) { g = x; b = c; } else if (h < 300) { r = x; b = c; } else { r = c; b = x; }
            return Color.FromArgb((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255));
        }

        static GraphicsPath Arredondado(Rectangle r, int raio)
        {
            GraphicsPath p = new GraphicsPath();
            int d = Math.Max(1, Math.Min(raio * 2, Math.Min(r.Width, r.Height)));
            p.AddArc(r.X, r.Y, d, d, 180, 90);
            p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            p.CloseFigure();
            return p;
        }
    }
}
