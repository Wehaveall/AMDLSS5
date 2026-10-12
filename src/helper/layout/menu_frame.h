#pragma once
#include <string>
#include <vector>
#include "geometry.h"
#include "menu_order.h"
#include "menu_i18n.h"

namespace nrmenulayout {
struct NativeView { const char* label; const char* help; };
inline Order catalog_order(const std::vector<NativeView>& rows) {
    std::vector<int> rank(rows.size());std::vector<bool> shown(rows.size());
    for(std::size_t i=0;i<rows.size();++i){rank[i]=nrmenu::catalog_index(rows[i].label);shown[i]=nrmenu::catalog_shown(rows[i].label);}
    return make_order(rank,shown);
}

template<class ValueOf>
Frame compose_frame(const std::vector<NativeView>& rows,const Order& order,unsigned selected_native,nrmenu::Language lang,ValueOf value_of) {
    Frame f;f.rows.reserve(order.display_to_native.size());
    for(const unsigned index:order.display_to_native){
        const auto& r=rows[index];
        const auto* entry=nrmenu::catalog_find(r.label);
        const std::string raw=value_of(index);

        std::string help=nrmenu::help(r.label,r.help,lang);

        if(raw.find("pend.")!=std::string::npos||raw=="Cinemático...")
            help=std::string(nrmenu::ui(nrmenu::Ui::PendingLegend,lang))+(help.empty()?"":"\n")+help;
        f.rows.push_back({nrmenu::label(r.label,lang),nrmenu::value(r.label,raw,lang),help,
                          entry?nrmenu::group_name(entry->group,lang):std::string()});
    }
    f.selected=selected_native<order.native_to_display.size()&&order.native_to_display[selected_native]>=0?
        unsigned(order.native_to_display[selected_native]):0;
    f.fallback_status=nrmenu::ui(nrmenu::Ui::FallbackStatus,lang);
    f.empty=nrmenu::ui(nrmenu::Ui::Empty,lang);
    f.hints=nrmenu::ui(nrmenu::Ui::Hints,lang);
    f.show_help=nrmenu::show_help();
    if(!f.rows.empty())f.position=std::to_string(f.selected+1)+"/"+std::to_string(f.rows.size());
    return f;
}
}
