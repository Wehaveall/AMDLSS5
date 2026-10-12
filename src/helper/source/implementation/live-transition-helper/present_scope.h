#pragma once
namespace nrpresent {
struct Scope {
    void* base; void* swapchain; void* queue{}; Scope* previous; bool processed{};
    inline static thread_local Scope* active{};
    Scope(void* b,void* s):base(b),swapchain(s),previous(active){active=this;}
    ~Scope(){active=previous;}
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
    static void record(void* b,void* s,void* q){if(active&&active->base==b&&active->swapchain==s)active->queue=q;}
    static void* selected(void* b,void* s){return active&&active->base==b&&active->swapchain==s?active->queue:nullptr;}
    static bool claim(void* b,void* s){
        if(!active||active->base!=b||active->swapchain!=s||active->processed)return false;
        for(auto* p=active->previous;p;p=p->previous)if(p->base==b&&p->swapchain==s)return false;
        active->processed=true;return true;
    }
};
}
