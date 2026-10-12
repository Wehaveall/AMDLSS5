#include "nr_publico.h"
#include "nr_publico_padroes.h"
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace nrcfg {
namespace {

unsigned long long fnv(const wchar_t* s) noexcept {
    unsigned long long h=1469598103934665603ull;
    for(;s&&*s;++s){
        unsigned c=unsigned(*s);
        if(c>='A'&&c<='Z')c+=32u;
        h^=c;h*=1099511628211ull;
    }
    return h;
}

constexpr const wchar_t* proxy[]={
    L"Enabled",L"LocalTone",L"LocalStructure",L"SkinStructure",L"UseAutoMask",L"ToneChannels",L"Scale",L"Temporal",
    L"Tonemap",L"UseFsrInputs",L"UseDepth",L"Interop",L"Inline",L"InlineWaitMs",L"HipDevice",L"PredWait",L"PredSlice"};

constexpr const wchar_t* ini_opcional[]={L"MenuLanguage"};
constexpr const wchar_t secao_real[]=L"DlssNrOnAmd";
constexpr const wchar_t nome_dat[]=L"amdlss5.dat";
const wchar_t* nome_proxy(unsigned long long h) noexcept {
    for(const auto* p:proxy)if(fnv(p)==h)return p;
    return nullptr;
}
bool hexa(wchar_t c,unsigned& v) noexcept {
    if(c>=L'0'&&c<=L'9'){v=unsigned(c-L'0');return true;}
    if(c>=L'a'&&c<=L'f'){v=unsigned(c-L'a'+10);return true;}
    if(c>=L'A'&&c<=L'F'){v=unsigned(c-L'A'+10);return true;}
    return false;
}
unsigned long long hash_de(const wchar_t* k) noexcept {return k?fnv(k):0;}

constexpr std::uint32_t magica=0x4C35A0D3u;
constexpr std::uint16_t versao=1;
std::uint32_t soma(const std::vector<unsigned char>& b,std::size_t n) noexcept {
    std::uint32_t h=2166136261u;for(std::size_t i=0;i<n;++i){h^=b[i];h*=16777619u;}return h;
}
void misturar(std::vector<unsigned char>& b) noexcept {
    std::uint64_t x=0x6A09E667F3BCC909ull^(std::uint64_t(b.size())*0x9E3779B97F4A7C15ull);
    for(auto& c:b){x^=x>>12;x^=x<<25;x^=x>>27;c^=(unsigned char)((x*0x2545F4914F6CDD1Dull)>>56);}
}
constexpr char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string para_b64(const std::vector<unsigned char>& b){
    std::string s;s.reserve((b.size()+2)/3*4);
    for(std::size_t i=0;i<b.size();i+=3){
        const unsigned v=(unsigned(b[i])<<16)|(i+1<b.size()?unsigned(b[i+1])<<8:0u)|(i+2<b.size()?unsigned(b[i+2]):0u);
        s+=b64[(v>>18)&63];s+=b64[(v>>12)&63];s+=i+1<b.size()?b64[(v>>6)&63]:'=';s+=i+2<b.size()?b64[v&63]:'=';
    }
    return s;
}
bool de_b64(const std::string& s,std::vector<unsigned char>& b){
    b.clear();unsigned v=0;int n=0;
    for(char c:s){
        if(c=='\r'||c=='\n'||c==' ')continue;
        if(c=='=')break;
        const char* p=std::find(b64,b64+64,c);if(p==b64+64)return false;
        v=(v<<6)|unsigned(p-b64);n+=6;
        if(n>=8){n-=8;b.push_back((unsigned char)((v>>n)&255u));}
    }
    return true;
}
struct Loja {
    std::mutex m;
    bool carregada{};
    std::wstring pasta,arquivo;
    std::unordered_map<unsigned long long,std::wstring> usuario;

    std::unordered_map<unsigned long long,const wchar_t*> padrao;
    std::unordered_map<unsigned long long,std::wstring> ini;
    unsigned long long jogo{};
};
Loja& loja() noexcept {static Loja* l=new Loja;return *l;}
std::wstring pasta_de(const std::wstring& caminho){
    const auto p=caminho.find_last_of(L"\\/");
    return p==std::wstring::npos?std::wstring{}:caminho.substr(0,p);
}
bool absoluto(const wchar_t* c) noexcept {
    return c&&((c[0]&&c[1]==L':'&&(c[2]==L'\\'||c[2]==L'/'))||(c[0]==L'\\'&&c[1]==L'\\'));
}
std::wstring pasta_do_modulo(){
    HMODULE self{};wchar_t caminho[MAX_PATH*4]{};
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&pasta_do_modulo),&self)&&
       GetModuleFileNameW(self,caminho,DWORD(sizeof(caminho)/sizeof(*caminho))))return pasta_de(caminho);
    return {};
}
unsigned long long jogo_atual(){
    wchar_t caminho[MAX_PATH*4]{};
    if(!GetModuleFileNameW(nullptr,caminho,DWORD(sizeof(caminho)/sizeof(*caminho))))return 0;
    const wchar_t* nome=caminho;for(const wchar_t* p=caminho;*p;++p)if(*p==L'\\'||*p==L'/')nome=p+1;
    return fnv(nome);
}
const wchar_t* idioma_windows() noexcept {
    switch(PRIMARYLANGID(GetUserDefaultUILanguage())){
        case LANG_PORTUGUESE:return L"pt";
        case LANG_SPANISH:return L"es";
        default:return L"en";
    }
}
void carregar_travado(Loja& l,const wchar_t* arquivo_ini){
    if(l.carregada)return;
    l.carregada=true;
    try{
        l.jogo=jogo_atual();
        if(l.pasta.empty()&&absoluto(arquivo_ini))l.pasta=pasta_de(arquivo_ini);
        if(l.pasta.empty())l.pasta=pasta_do_modulo();
        l.arquivo=l.pasta.empty()?std::wstring(nome_dat):l.pasta+L'\\'+nome_dat;

        l.padrao[fnv(L"MenuLanguage")]=idioma_windows();
        for(const auto& p:padroes::itens)if(!p.jogo)l.padrao[fnv(p.chave)]=p.valor;
        for(const auto& p:padroes::itens)if(p.jogo&&fnv(p.jogo)==l.jogo)l.padrao[fnv(p.chave)]=p.valor;
        if(absoluto(arquivo_ini))for(const auto* k:ini_opcional){
            wchar_t t[256]{};
            const DWORD n=GetPrivateProfileStringW(secao_real,k,L"",t,256,arquivo_ini);
            if(n)l.ini[fnv(k)]=std::wstring(t,n);
        }
        const HANDLE h=CreateFileW(l.arquivo.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)return;
        std::string texto;LARGE_INTEGER t{};
        if(GetFileSizeEx(h,&t)&&t.QuadPart>0&&t.QuadPart<(1<<20)){
            texto.resize(std::size_t(t.QuadPart));DWORD lido=0;
            if(!ReadFile(h,texto.data(),DWORD(texto.size()),&lido,nullptr))lido=0;
            texto.resize(lido);
        }
        CloseHandle(h);
        std::vector<unsigned char> b;
        if(!de_b64(texto,b)||b.size()<12)return;
        misturar(b);
        const auto u16=[&](std::size_t o){return std::uint16_t(b[o]|(b[o+1]<<8));};
        const auto u32=[&](std::size_t o){return std::uint32_t(b[o])|(std::uint32_t(b[o+1])<<8)|(std::uint32_t(b[o+2])<<16)|(std::uint32_t(b[o+3])<<24);};
        if(u32(0)!=magica||u16(4)!=versao||u32(b.size()-4)!=soma(b,b.size()-4))return;
        const unsigned n=u16(6);std::size_t o=8;
        std::unordered_map<unsigned long long,std::wstring> lidos;
        for(unsigned i=0;i<n;++i){
            if(o+10>b.size()-4)return;
            unsigned long long h64=0;for(int k=7;k>=0;--k)h64=(h64<<8)|b[o+std::size_t(k)];
            const std::size_t len=u16(o+8);o+=10;
            if(o+len*2>b.size()-4)return;
            std::wstring v(len,L'\0');for(std::size_t k=0;k<len;++k)v[k]=wchar_t(u16(o+2*k));
            o+=len*2;lidos[h64]=std::move(v);
        }
        if(o!=b.size()-4)return;
        l.usuario=std::move(lidos);
    }catch(...){l.usuario.clear();}
}
bool salvar_travado(Loja& l){
    try{
        std::vector<std::pair<unsigned long long,const std::wstring*>> itens;
        for(const auto& kv:l.usuario)itens.emplace_back(kv.first,&kv.second);
        std::sort(itens.begin(),itens.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        std::vector<unsigned char> b;
        const auto p16=[&](unsigned v){b.push_back((unsigned char)(v&255u));b.push_back((unsigned char)((v>>8)&255u));};
        const auto p32=[&](std::uint32_t v){for(int k=0;k<4;++k)b.push_back((unsigned char)((v>>(8*k))&255u));};
        p32(magica);p16(versao);p16(unsigned(itens.size()));
        for(const auto& it:itens){
            for(int k=0;k<8;++k)b.push_back((unsigned char)((it.first>>(8*k))&255u));
            const std::size_t len=std::min<std::size_t>(it.second->size(),255);
            p16(unsigned(len));for(std::size_t k=0;k<len;++k)p16(unsigned((*it.second)[k]));
        }
        p32(soma(b,b.size()));
        misturar(b);
        const std::string texto=para_b64(b);
        const std::wstring temp=l.arquivo+L".tmp";
        const HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)return false;
        DWORD escrito=0;const bool ok=WriteFile(h,texto.data(),DWORD(texto.size()),&escrito,nullptr)&&escrito==texto.size();
        CloseHandle(h);
        if(!ok||!MoveFileExW(temp.c_str(),l.arquivo.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());return false;}
        return true;
    }catch(...){return false;}
}

bool valor_travado(Loja& l,unsigned long long h,std::wstring& v){
    const auto it=l.usuario.find(h);
    if(it!=l.usuario.end()){v=it->second;return true;}
    {const auto o=l.ini.find(h);if(o!=l.ini.end()){v=o->second;return true;}}
    const auto p=l.padrao.find(h);
    if(p!=l.padrao.end()){v=p->second;return true;}
    return false;
}
DWORD copiar(const wchar_t* v,LPWSTR saida,DWORD n) noexcept {
    if(!saida||!n)return 0;
    std::size_t len=v?std::wcslen(v):0;if(len>n-1)len=n-1;
    if(len)std::wmemcpy(saida,v,len);saida[len]=L'\0';
    return DWORD(len);
}

UINT inteiro(const std::wstring& t) noexcept {
    std::size_t i=0;while(i<t.size()&&(t[i]==L' '||t[i]==L'\t'))++i;
    bool neg=false;if(i<t.size()&&(t[i]==L'-'||t[i]==L'+')){neg=t[i]==L'-';++i;}
    unsigned long long v=0;
    if(i+1<t.size()&&t[i]==L'0'&&(t[i+1]==L'x'||t[i+1]==L'X')){
        i+=2;unsigned d=0;while(i<t.size()&&hexa(t[i],d)){v=(v<<4)|d;++i;}
    }else while(i<t.size()&&t[i]>=L'0'&&t[i]<=L'9'){v=v*10u+unsigned(t[i]-L'0');++i;}
    const UINT u=UINT(v&0xFFFFFFFFull);
    return neg?UINT(0u-u):u;
}
}

void definir_ini(const wchar_t* caminho) noexcept {
    try{
        if(!absoluto(caminho))return;
        auto& l=loja();std::lock_guard<std::mutex> t(l.m);
        if(!l.carregada)l.pasta=pasta_de(caminho);
    }catch(...){}
}
UINT ler_int(LPCWSTR secao,LPCWSTR chave,INT padrao,LPCWSTR arquivo) noexcept {
    (void)secao;
    try{
        const auto h=hash_de(chave);
        if(const wchar_t* nome=nome_proxy(h))return GetPrivateProfileIntW(secao_real,nome,padrao,arquivo);
        auto& l=loja();std::lock_guard<std::mutex> t(l.m);carregar_travado(l,arquivo);
        std::wstring v;if(!valor_travado(l,h,v))return UINT(padrao);
        return inteiro(v);
    }catch(...){return UINT(padrao);}
}
DWORD ler_texto(LPCWSTR secao,LPCWSTR chave,LPCWSTR padrao,LPWSTR saida,DWORD n,LPCWSTR arquivo) noexcept {
    (void)secao;
    try{
        if(!chave)return copiar(L"",saida,n);
        const auto h=hash_de(chave);
        if(const wchar_t* nome=nome_proxy(h))return GetPrivateProfileStringW(secao_real,nome,padrao,saida,n,arquivo);
        auto& l=loja();std::lock_guard<std::mutex> t(l.m);carregar_travado(l,arquivo);
        std::wstring v;if(!valor_travado(l,h,v))return copiar(padrao,saida,n);
        return copiar(v.c_str(),saida,n);
    }catch(...){return copiar(padrao,saida,n);}
}
BOOL escrever(LPCWSTR secao,LPCWSTR chave,LPCWSTR valor,LPCWSTR arquivo) noexcept {
    (void)secao;
    try{
        if(!chave){SetLastError(ERROR_SUCCESS);return TRUE;}
        const auto h=hash_de(chave);
        if(const wchar_t* nome=nome_proxy(h))return WritePrivateProfileStringW(secao_real,nome,valor,arquivo);
        auto& l=loja();std::lock_guard<std::mutex> t(l.m);carregar_travado(l,arquivo);
        if(valor)l.usuario[h]=valor;else l.usuario.erase(h);
        return salvar_travado(l)?TRUE:FALSE;
    }catch(...){return FALSE;}
}
unsigned completar_ini_proxy(const wchar_t* ini,const wchar_t* jogo) noexcept {
    try{
        if(!absoluto(ini))return 0;
        const unsigned long long este=jogo?fnv(jogo):jogo_atual();
        unsigned gravadas=0;
        for(const auto& p:padroes::proxy){
            if(p.jogo)continue;
            const wchar_t* valor=p.valor;
            for(const auto& q:padroes::proxy)if(q.jogo&&fnv(q.jogo)==este&&fnv(q.chave)==fnv(p.chave))valor=q.valor;

            wchar_t t[64]{};
            const DWORD n=GetPrivateProfileStringW(secao_real,p.chave,L"\x01",t,64,ini);
            if(!(n==1&&t[0]==L'\x01'))continue;
            if(WritePrivateProfileStringW(secao_real,p.chave,valor,ini))++gravadas;
        }
        return gravadas;
    }catch(...){return 0;}
}
}
