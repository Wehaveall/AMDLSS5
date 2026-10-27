#pragma once

namespace nrestab {
enum Fonte : unsigned {Nenhuma=0,Menu=1,Ini=2,FgTroca=3,Crossfade=4};
struct Base {float alfa;bool sem_limite;Fonte fonte;};

inline Base base(float menu,bool post,float post_estab) noexcept {
    if(menu>0.f&&menu<1.f)return {menu,false,Menu};
    if(post&&post_estab>0.f)return {post_estab,true,Ini};
    return {menu,false,Nenhuma};
}

inline float luz_parada(float menu_luz,float post_estab_luz,bool sem_limite_parado,bool segurar,Fonte* f=nullptr) noexcept {
    const bool ini=post_estab_luz>0.f&&!segurar&&sem_limite_parado;if(f)*f=ini?Ini:Menu;
    return ini?post_estab_luz:menu_luz;
}

inline float luz_andando(bool troca,bool fg,float fg_troca_luz,bool fg_troca_sempre,float post_luz_mov,Fonte* f=nullptr) noexcept {
    const bool t=troca&&fg_troca_luz>0.f&&(fg_troca_sempre||fg);if(f)*f=t?FgTroca:Ini;
    return t?fg_troca_luz:post_luz_mov;
}
inline const char* nome(Fonte f) noexcept {return f==Menu?"menu":f==Ini?"INI":f==FgTroca?"FgTrocaLuz":f==Crossfade?"crossfade":"nenhuma";}
}
