// Leitura minima de PE (Portable Executable): maquina, se e DLL, DLLs importadas e nomes exportados.
// Le so os cabecalhos e as tabelas por seek (exes de jogo passam de 200 MB). Veio do mod/Install-Game.ps1
// (Get-PEHeader / Get-PEImportedDlls; o script saiu em 22/10, review M-12: este e o unico leitor).
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Amdlss5.Instalador
{
    public sealed class InfoPe
    {
        public ushort Maquina;
        public bool EhDll;
        public List<string> Importa = new List<string>();
        public List<string> Exporta = new List<string>();
        public bool X64 { get { return Maquina == 0x8664; } }

        public bool ImportaDll(string nome)
        {
            foreach (string s in Importa) if (string.Equals(s, nome, StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }

        public bool ExportaNome(string nome)
        {
            foreach (string s in Exporta) if (string.Equals(s, nome, StringComparison.Ordinal)) return true;
            return false;
        }
    }

    public static class Pe
    {
        sealed class Secao { public uint Va, VSize, Raw, RawSize; }

        // null se nao e PE valido. Nunca lanca excecao (arquivo travado/malformado -> null).
        public static InfoPe Ler(string caminho)
        {
            try
            {
                using (FileStream fs = new FileStream(caminho, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                using (BinaryReader r = new BinaryReader(fs))
                {
                    if (fs.Length < 0x40) return null;
                    if (r.ReadUInt16() != 0x5A4D) return null; // MZ
                    fs.Seek(0x3C, SeekOrigin.Begin);
                    int lfanew = r.ReadInt32();
                    if (lfanew <= 0 || lfanew > fs.Length - 24) return null;
                    fs.Seek(lfanew, SeekOrigin.Begin);
                    if (r.ReadUInt32() != 0x00004550) return null; // PE\0\0
                    InfoPe info = new InfoPe();
                    info.Maquina = r.ReadUInt16();
                    ushort nSecoes = r.ReadUInt16();
                    fs.Seek(12, SeekOrigin.Current);
                    ushort tamOpt = r.ReadUInt16();
                    ushort caracteristicas = r.ReadUInt16();
                    info.EhDll = (caracteristicas & 0x2000) != 0;
                    long opt = fs.Position;
                    ushort magic = r.ReadUInt16();
                    bool pe32mais = magic == 0x20B;
                    long numRva = opt + (pe32mais ? 108 : 92);
                    fs.Seek(numRva, SeekOrigin.Begin);
                    uint nDirs = r.ReadUInt32();
                    uint expRva = 0, impRva = 0;
                    if (nDirs >= 1) { expRva = r.ReadUInt32(); r.ReadUInt32(); }
                    if (nDirs >= 2) { impRva = r.ReadUInt32(); r.ReadUInt32(); }
                    fs.Seek(opt + tamOpt, SeekOrigin.Begin);
                    List<Secao> secoes = new List<Secao>();
                    for (int i = 0; i < nSecoes && i < 96; i++)
                    {
                        long b = opt + tamOpt + i * 40;
                        fs.Seek(b + 8, SeekOrigin.Begin);
                        Secao s = new Secao();
                        s.VSize = r.ReadUInt32(); s.Va = r.ReadUInt32(); s.RawSize = r.ReadUInt32(); s.Raw = r.ReadUInt32();
                        secoes.Add(s);
                    }
                    if (impRva != 0)
                    {
                        long off = RvaParaOffset(secoes, impRva);
                        for (int i = 0; off >= 0 && i < 4096; i++)
                        {
                            fs.Seek(off + i * 20, SeekOrigin.Begin);
                            uint a = r.ReadUInt32(), t = r.ReadUInt32(), f = r.ReadUInt32(), nomeRva = r.ReadUInt32(), ft = r.ReadUInt32();
                            if (a == 0 && t == 0 && f == 0 && nomeRva == 0 && ft == 0) break;
                            string n = LerTexto(fs, RvaParaOffset(secoes, nomeRva));
                            if (!string.IsNullOrEmpty(n)) info.Importa.Add(n);
                        }
                    }
                    if (expRva != 0)
                    {
                        long off = RvaParaOffset(secoes, expRva);
                        if (off >= 0)
                        {
                            fs.Seek(off + 24, SeekOrigin.Begin);
                            uint nNomes = r.ReadUInt32();
                            r.ReadUInt32();
                            uint nomesRva = r.ReadUInt32();
                            long nomesOff = RvaParaOffset(secoes, nomesRva);
                            for (uint i = 0; nomesOff >= 0 && i < nNomes && i < 4096; i++)
                            {
                                fs.Seek(nomesOff + i * 4, SeekOrigin.Begin);
                                uint nr = r.ReadUInt32();
                                string n = LerTexto(fs, RvaParaOffset(secoes, nr));
                                if (!string.IsNullOrEmpty(n)) info.Exporta.Add(n);
                            }
                        }
                    }
                    return info;
                }
            }
            catch (Exception) { return null; }
        }

        static long RvaParaOffset(List<Secao> secoes, uint rva)
        {
            foreach (Secao s in secoes)
            {
                uint tam = Math.Max(s.VSize, s.RawSize);
                if (rva >= s.Va && rva < s.Va + tam) return (long)rva - s.Va + s.Raw;
            }
            return -1;
        }

        static string LerTexto(FileStream fs, long off)
        {
            if (off < 0 || off >= fs.Length) return null;
            fs.Seek(off, SeekOrigin.Begin);
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 512; i++)
            {
                int b = fs.ReadByte();
                if (b <= 0) break;
                sb.Append((char)b);
            }
            return sb.ToString();
        }
    }
}
