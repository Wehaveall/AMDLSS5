// Classificacao: cada jogo achado nas lojas cai em um grupo, para a biblioteca mostrar a etiqueta e os botoes certos.
//   Testado       esta na lista payload\testados.json (loja+id, ou so o nome do exe quando a entrada nao tem id)
//   NaoTestado    o mod pode ser instalado (FSR 3.1, DX12, x64), mas o jogo nao esta na lista
//   NaoSuportado  bloqueio do detector, sem FSR 3.1, sem DX12, com anticheat ou em WindowsApps (nao instala)
using System;
using System.Collections.Generic;
using System.IO;

namespace Amdlss5.Instalador
{
    public enum Grupo { Testado, NaoTestado, NaoSuportado }

    public sealed class Classificado
    {
        public JogoLoja Loja;
        public Jogo Jogo;
        public Grupo Grupo;
        public string Motivo = "";   // texto ja traduzido; vazio no Testado
        public string ExeNome;       // nome do exe do jogo; null = a pasta nao tem exe de jogo (a biblioteca nao a mostra)
        public bool Anticheat;       // anticheat achado: a biblioteca poe a tarja vermelha na capa (jogo online competitivo: risco de banimento)
    }

    public static class Classificacao
    {
        // 21/10 (Battlefield 6 aparecia "ainda nao testado": EA AntiCheat/Javelin = pasta EAAntiCheat, EAAntiCheat.*.exe): alem destas, qualquer
        // pasta ou arquivo com "anticheat"/"anti-cheat"/"anti_cheat" no nome (NomeAnticheat).
        static readonly string[] PastasAnticheat = { "easyanticheat", "easyanticheat_eos", "battleye", "gameguard", "xigncode", "xigncode3", "punkbuster", "nprotect" };

        public static string NomeGrupo(Grupo g)   // chave do --listar (ingles, estavel)
        {
            return g == Grupo.Testado ? "tested" : g == Grupo.NaoTestado ? "not_tested" : "not_supported";
        }

        public static string TextoGrupo(Grupo g)
        {
            return T.S(g == Grupo.Testado ? "grupo_testado" : g == Grupo.NaoTestado ? "grupo_nao_testado" : "grupo_nao_suportado");
        }

        // Jogos de todas as lojas, ja classificados, sem as pastas que nao tem exe de jogo (o --listar e a biblioteca
        // usam esta lista). Um jogo cuja deteccao falha vira a linha "JOGO_ERRO: <pasta>: <mensagem>" e os outros seguem.
        public static List<Classificado> ListarClassificados(OpcoesLojas o, Payload p, IRelato r)
        {
            List<Classificado> saida = new List<Classificado>();
            foreach (JogoLoja j in Lojas.Listar(o, r))
            {
                Classificado c;
                try { c = Classificar(j, p); }
                catch (Exception e) { r.Linha("JOGO_ERRO: " + j.Pasta + ": " + e.Message); continue; }
                if (c.ExeNome != null) saida.Add(c);
            }
            return saida;
        }

        // p pode ser null (pacote ausente): nada e "testado".
        public static Classificado Classificar(JogoLoja l, Payload p)
        {
            Classificado c = new Classificado();
            c.Loja = l;
            c.Jogo = Detector.Detectar(l.Pasta, p);
            Jogo j = c.Jogo;
            if (j.Exe != null) c.ExeNome = Path.GetFileName(j.Exe);
            else
            {
                // sem exe x64: so um exe de 32 bits mostra que a pasta e de um jogo (que o mod nao atende); senao nao e jogo
                string x86 = Detector.Exe32Bits(l.Pasta);
                if (x86 == null) { c.Grupo = Grupo.NaoSuportado; return c; }
                c.ExeNome = Path.GetFileName(x86);
                c.Grupo = Grupo.NaoSuportado; c.Motivo = T.F("det_nao_x64", c.ExeNome); return c;
            }
            if (TemAnticheat(j.PastaMod, l.Pasta)) { c.Grupo = Grupo.NaoSuportado; c.Anticheat = true; c.Motivo = T.S("motivo_anticheat"); return c; }
            if (j.Bloqueios.Count > 0) { c.Grupo = Grupo.NaoSuportado; c.Motivo = j.Bloqueios[0]; return c; }
            if (j.FfxTipo == "nenhum") { c.Grupo = Grupo.NaoSuportado; c.Motivo = T.S("det_sem_fsr"); return c; }
            if (!j.Dx12) { c.Grupo = Grupo.NaoSuportado; c.Motivo = T.S("det_sem_dx12"); return c; }
            if (EmWindowsApps(l.Pasta)) { c.Grupo = Grupo.NaoSuportado; c.Motivo = T.S("motivo_windowsapps"); return c; }
            if (p != null && p.EhTestado(l.Loja, l.Id, j.Exe)) { c.Grupo = Grupo.Testado; return c; }
            c.Grupo = Grupo.NaoTestado;
            if (j.Avisos.Count > 0) c.Motivo = string.Join(" ", j.Avisos.ToArray());   // ex.: FSR nao reconhecido (FfxTipo "outro")
            return c;
        }

        static bool EmWindowsApps(string pasta)
        {
            foreach (string seg in (pasta ?? "").Split('\\', '/'))
                if (string.Equals(seg, "WindowsApps", StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }

        // Anticheat no jogo: a pasta da loja (ou a escolhida a mao) ate 2 niveis abaixo dela, e tambem a pasta do exe
        // (pastaMod, que pode estar 3 pastas abaixo) e cada pasta entre ela e a da loja, cada uma ate 2 niveis abaixo.
        // Usado pela biblioteca (Classificar) e pela janela "adicionar jogo manualmente" (bloqueia o Instalar).
        internal static bool TemAnticheat(string pastaMod, string pastaLoja)
        {
            List<string> raizes = new List<string>();
            try
            {
                string loja = string.IsNullOrEmpty(pastaLoja) ? null : Path.GetFullPath(pastaLoja).TrimEnd('\\', '/');
                if (!string.IsNullOrEmpty(pastaMod))
                {
                    string d = Path.GetFullPath(pastaMod).TrimEnd('\\', '/');
                    raizes.Add(d);
                    // sobe ate a pasta da loja (sem ela: entra no fim); fora dela, so a pasta do exe
                    if (loja != null && (d + "\\").StartsWith(loja + "\\", StringComparison.OrdinalIgnoreCase))
                        for (string mae = Path.GetDirectoryName(d); mae != null && mae.TrimEnd('\\', '/').Length > loja.Length; mae = Path.GetDirectoryName(mae))
                            raizes.Add(mae.TrimEnd('\\', '/'));
                }
                if (loja != null) raizes.Add(loja);
            }
            catch (Exception) { }   // caminho invalido: varre o que deu para montar
            Dictionary<string, int> vistas = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            foreach (string r in raizes)
            {
                try { if (Directory.Exists(r) && Varrer(r, 0, vistas)) return true; }
                catch (Exception) { }   // pasta ilegivel: o detector ja reclama do resto
            }
            return false;
        }

        static bool NomeAnticheat(string n) { return n.Contains("anticheat") || n.Contains("anti-cheat") || n.Contains("anti_cheat"); }

        // marcas de anticheat: nome com "anticheat" (EasyAntiCheat, EA AntiCheat, AntiCheatExpert...), BattlEye (BEService*.exe,
        // BEClient*.dll), PunkBuster (pbsvc.exe), GameGuard (gamemon*.des), XIGNCODE (x3.xem), miHoYo (mhyprot*.sys), o lancador protegido
        static bool ArquivoAnticheat(string nome)
        {
            string n = nome.ToLowerInvariant();
            if (NomeAnticheat(n)) return true;
            if (n == "pbsvc.exe" || n == "x3.xem" || (n.StartsWith("gamemon") && n.EndsWith(".des")) || (n.StartsWith("mhyprot") && n.EndsWith(".sys"))) return true;
            if (n.StartsWith("easyanticheat") && (n.EndsWith(".dll") || n.EndsWith(".sys"))) return true;
            if (n.StartsWith("easyanticheat_") && n.EndsWith(".exe")) return true;
            if (n.StartsWith("beservice") && n.EndsWith(".exe")) return true;
            if (n.StartsWith("beclient") && n.EndsWith(".dll")) return true;
            return n == "start_protected_game.exe";
        }

        // vistas: pasta -> menor nivel em que ja foi varrida (de novo so se agora for varrer mais fundo)
        static bool Varrer(string dir, int nivel, Dictionary<string, int> vistas)
        {
            int antes;
            if (vistas.TryGetValue(dir, out antes) && antes <= nivel) return false;
            vistas[dir] = nivel;
            foreach (string f in Directory.GetFiles(dir))
                if (ArquivoAnticheat(Path.GetFileName(f))) return true;
            if (nivel >= 2) return false;
            foreach (string d in Directory.GetDirectories(dir))
            {
                string pd = Path.GetFileName(d).ToLowerInvariant();
                if (Array.IndexOf(PastasAnticheat, pd) >= 0 || NomeAnticheat(pd)) return true;
                if (Varrer(d, nivel + 1, vistas)) return true;
            }
            return false;
        }
    }
}
