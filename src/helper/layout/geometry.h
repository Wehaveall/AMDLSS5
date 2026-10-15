#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace nrmenulayout {
struct Vertex { float x,y,u,v,r,g,b,a; };
static_assert(sizeof(Vertex)==32);
struct Row { std::string label,value,help,group; };
struct Color { float r,g,b,a; };

namespace palette {
inline constexpr Color brand_red{.929f,.110f,.141f,1},brand_green{.463f,.725f,0.f,1};
inline constexpr Color title_neutral{.86f,.88f,.91f,1};
inline constexpr Color background{.030f,.034f,.043f,.76f};
inline constexpr Color text{.88f,.90f,.93f,1},muted{.60f,.64f,.70f,1},faint{.46f,.50f,.56f,1};
inline constexpr Color selected_text{1,1,1,1},selected_value{.60f,.84f,.16f,1},value{.70f,.75f,.80f,1};
inline constexpr Color selection{1,1,1,.075f},hairline{1,1,1,.10f},track{1,1,1,.06f},thumb{1,1,1,.28f};
inline constexpr Color fps{.95f,.96f,.97f,1};
}

struct Codepoint { char base; char accent; bool flip=false; bool raised=false; };
inline Codepoint decode(const std::string& text,std::size_t& i) {
    unsigned c=static_cast<unsigned char>(text[i++]);
    if(c<128)return {char(c),0};
    if(c==0xc3&&i<text.size()) {
        unsigned d=static_cast<unsigned char>(text[i++]);
        switch(d) {
        case 0x80:return {'A','`'};case 0x81:return {'A','\''};case 0x82:return {'A','^'};case 0x83:return {'A','~'};
        case 0x87:return {'C',','};case 0x88:return {'E','`'};case 0x89:return {'E','\''};case 0x8a:return {'E','^'};
        case 0x8c:return {'I','`'};case 0x8d:return {'I','\''};case 0x91:return {'N','~'};case 0x92:return {'O','`'};
        case 0x93:return {'O','\''};case 0x94:return {'O','^'};case 0x95:return {'O','~'};case 0x99:return {'U','`'};
        case 0x9a:return {'U','\''};case 0x9c:return {'U','"'};
        case 0xa0:return {'a','`'};case 0xa1:return {'a','\''};case 0xa2:return {'a','^'};case 0xa3:return {'a','~'};
        case 0xa7:return {'c',','};case 0xa8:return {'e','`'};case 0xa9:return {'e','\''};case 0xaa:return {'e','^'};
        case 0xac:return {'i','`'};case 0xad:return {'i','\''};case 0xb1:return {'n','~'};case 0xb2:return {'o','`'};
        case 0xb3:return {'o','\''};case 0xb4:return {'o','^'};case 0xb5:return {'o','~'};case 0xb9:return {'u','`'};
        case 0xba:return {'u','\''};case 0xbc:return {'u','"'};
        default:return {'?',0};
        }
    }
    if(c==0xc2&&i<text.size()) {
        unsigned d=static_cast<unsigned char>(text[i++]);

        if(d==0xbf)return {'?',0,true};
        if(d==0xa1)return {'!',0,true};
        if(d==0xb7)return {'.',0,false,true};
        if(d==0xb0)return {'o',0,false,true};
        if(d==0xa0)return {' ',0};
        return {'?',0};
    }
    if(c==0xe2&&i+1<text.size()&&static_cast<unsigned char>(text[i])==0x80) {
        unsigned d=static_cast<unsigned char>(text[i+1]);i+=2;
        if(d==0x93||d==0x94)return {'-',0};
        if(d==0x98||d==0x99)return {'\'',0};
        if(d==0x9c||d==0x9d)return {'"',0};
        if(d==0xa2)return {'.',0,false,true};
        if(d==0xa6)return {'.',0};
        return {'?',0};
    }
    while(i<text.size()&&(static_cast<unsigned char>(text[i])&0xc0)==0x80)++i;
    return {'?',0};
}
inline std::size_t columns_of(const std::string& text) {
    std::size_t n=0;for(std::size_t i=0;i<text.size();){decode(text,i);++n;}return n;
}

inline std::string fit(const std::string& text,std::size_t columns) {
    if(columns_of(text)<=columns)return text;
    if(columns<3)return columns?std::string(columns,'.'):std::string();
    std::size_t i=0,n=0;
    while(i<text.size()&&n<columns-2){decode(text,i);++n;}
    std::string out=text.substr(0,i);
    while(!out.empty()&&out.back()==' ')out.pop_back();
    return out+"..";
}

inline Color network_ms_color(double ms) {
    constexpr Color green{.35f,.9f,.45f,1},yellow{1.f,.84f,.25f,1},red{1.f,.4f,.36f,1};
    return ms<25?green:ms<=50?yellow:red;
}

struct MsSpan { std::size_t begin=0,end=0; double ms=0; };
inline MsSpan network_ms_span(const std::string& status) {

    for(const std::string key:{"Tempo da rede neural: ","Neural network time: ","Tiempo de la red neuronal: "}) {
        const auto at=status.find(key);if(at==std::string::npos)continue;
        const std::size_t begin=at+key.size();std::size_t end=begin;
        while(end<status.size()&&((status[end]>='0'&&status[end]<='9')||status[end]=='.'))++end;
        if(end==begin||status.compare(end,3," ms")!=0)continue;
        return {begin,end+3,std::strtod(status.substr(begin,end-begin).c_str(),nullptr)};
    }
    for(const std::string key:{" tarefa "," job "," tarea "}) {
        const auto at=status.find(key);if(at==std::string::npos)continue;
        std::size_t i=at+key.size();const auto job=i;
        if(i<status.size()&&status[i]=='-')++i;
        while(i<status.size()&&status[i]>='0'&&status[i]<='9')++i;
        if(i==job||status.compare(i,2,": ")!=0)continue;
        const std::size_t begin=i+2;std::size_t end=begin;
        while(end<status.size()&&((status[end]>='0'&&status[end]<='9')||status[end]=='.'))++end;
        if(end==begin||status.compare(end,3," ms")!=0)continue;
        return {begin,end+3,std::strtod(status.substr(begin,end-begin).c_str(),nullptr)};
    }
    return {};
}
class Canvas {
public:
    std::vector<Vertex> vertices;
    float atlas_width=160,atlas_height=120,cell_width=10,cell_height=20;
    void quad(float x,float y,float w,float h,float u,float v,float uw,float vh,Color c) {
        const Vertex a{x,y,u,v,c.r,c.g,c.b,c.a},b{x+w,y,u+uw,v,c.r,c.g,c.b,c.a},
            d{x,y+h,u,v+vh,c.r,c.g,c.b,c.a},e{x+w,y+h,u+uw,v+vh,c.r,c.g,c.b,c.a};
        vertices.insert(vertices.end(),{a,b,d,b,e,d});
    }
    void rect(float x,float y,float w,float h,Color c) { if(w>0&&h>0)quad(x,y,w,h,-1,-1,0,0,c); }

    void cell(char c,float& u,float& v,float& uw,float& vh) const {
        const auto index=unsigned(std::clamp(int(static_cast<unsigned char>(c)),32,126)-32);
        u=float(index%16)*cell_width/atlas_width;v=float(index/16)*cell_height/atlas_height;
        uw=cell_width/atlas_width;vh=cell_height/atlas_height;
    }

    void glyph(char c,float x,float y,float scale,Color color,bool flip=false,
               float clip0=-std::numeric_limits<float>::infinity(),float clip1=std::numeric_limits<float>::infinity()) {
        if(c==' ')return;
        float u,v,uw,vh;cell(c,u,v,uw,vh);

        const float texel=vh/cell_height;
        v+=texel;vh-=2*texel;y+=scale;
        const float w=cell_width*scale,h=(cell_height-2)*scale;
        float u0=u,u1=u+uw,v0=v,v1=v+vh;
        if(flip){std::swap(u0,u1);std::swap(v0,v1);}
        const float x0=std::max(x,clip0),x1=std::min(x+w,clip1);
        if(!(x1>x0))return;
        const float t0=(x0-x)/w,t1=(x1-x)/w;
        const float cu0=u0+(u1-u0)*t0,cu1=u0+(u1-u0)*t1;
        quad(x0,y,x1-x0,h,cu0,v0,cu1-cu0,v1-v0,color);
    }

    void part(char c,float sx0,float sy0,float sx1,float sy1,float k,float dx,float dy,float x,float y,float scale,Color color) {
        float u,v,uw,vh;cell(c,u,v,uw,vh);
        const float fu=uw/cell_width,fv=vh/cell_height;
        quad(x+dx*scale,y+dy*scale,(sx1-sx0)*k*scale,(sy1-sy0)*k*scale,u+sx0*fu,v+sy0*fv,(sx1-sx0)*fu,(sy1-sy0)*fv,color);
    }

    void accent(char kind,bool upper,float x,float y,float scale,Color color) {
        const float lift=upper?-2.7f:0.f;
        switch(kind){
        case '\'':part('/',0,3,10,8.5f,.7f,.45f,1.45f+lift,x,y,scale,color);break;
        case '`':part('\\',0,3,10,8.5f,.7f,2.3f,1.45f+lift,x,y,scale,color);break;
        case '^':part('^',0,3,10,7.6f,.75f,1.1f,1.85f+lift,x,y,scale,color);break;
        case '~':part('~',0,8,10,11.6f,.8f,1.3f,2.5f+lift,x,y,scale,color);break;
        case '"':part('.',2.5f,11,5.5f,13.6f,.85f,1.6f,3.1f+lift,x,y,scale,color);
                 part('.',2.5f,11,5.5f,13.6f,.85f,5.0f,3.1f+lift,x,y,scale,color);break;
        case ',':part(',',0,11,6,16.5f,.7f,2.4f,13.2f,x,y,scale,color);break;
        default:break;
        }
    }
    void glyph(const Codepoint& cp,float x,float y,float scale,Color color) {
        if(cp.accent&&cp.base=='i')part('i',0,5.6f,10,19,1,0,5.6f,x,y,scale,color);
        else glyph(cp.base,x,y+(cp.raised?-2.5f*scale:0.f),scale,color,cp.flip);
        if(cp.accent)accent(cp.accent,cp.base>='A'&&cp.base<='Z',x,y,scale,color);
    }
    float advance(float scale) const { return cell_width*scale; }
    float line(float scale) const { return (cell_height+3)*scale; }

    void text(const std::string& text,float x,float y,float scale,Color color,unsigned columns,unsigned lines=1,
              std::size_t span_begin=0,std::size_t span_end=0,Color span_color={}) {
        if(!columns||!lines)return;
        std::vector<Codepoint> chars;chars.reserve(text.size());
        std::vector<std::size_t> starts;starts.reserve(text.size());
        for(std::size_t i=0;i<text.size();){starts.push_back(i);chars.push_back(decode(text,i));}
        unsigned col=0,row=0;
        for(std::size_t i=0;i<chars.size();++i) {
            auto cp=chars[i];
            const Color color_i=starts[i]>=span_begin&&starts[i]<span_end?span_color:color;
            if(cp.base=='\n'){col=0;if(++row>=lines)break;continue;}

            if(cp.base!=' '&&(i==0||chars[i-1].base==' ')) {
                std::size_t length=0;while(i+length<chars.size()&&chars[i+length].base!=' '&&chars[i+length].base!='\n')++length;
                if(col&&length<=columns&&col+length>columns){col=0;if(++row>=lines)break;}
            }
            if(col>=columns){col=0;if(++row>=lines)break;}
            if(!col&&cp.base==' ')continue;
            const float px=x+col*cell_width*scale,py=y+row*line(scale);
            if(row+1==lines&&col+1==columns&&i+1<chars.size()) {
                glyph('>',px,py,scale,color_i);break;
            }
            glyph(cp,px,py,scale,color_i);
            ++col;
        }
    }

    static unsigned wrapped_lines(const std::string& text,unsigned columns,unsigned max_lines) {
        if(text.empty()||!columns)return 0;
        std::vector<char> chars;for(std::size_t i=0;i<text.size();)chars.push_back(decode(text,i).base);
        unsigned col=0,row=0;
        for(std::size_t i=0;i<chars.size();++i) {
            if(chars[i]=='\n'){col=0;if(++row>=max_lines)return max_lines;continue;}
            if(chars[i]!=' '&&(i==0||chars[i-1]==' ')) {
                std::size_t length=0;while(i+length<chars.size()&&chars[i+length]!=' '&&chars[i+length]!='\n')++length;
                if(col&&length<=columns&&col+length>columns){col=0;if(++row>=max_lines)return max_lines;}
            }
            if(col>=columns){col=0;if(++row>=max_lines)return max_lines;}
            if(!col&&chars[i]==' ')continue;
            ++col;
        }
        return std::min(max_lines,row+1);
    }
};

struct TitleLayout { float x,y,scale,bold,split_x; };
inline constexpr char title_text[]="AMDLSS5 MOD";

inline constexpr char mod_version[]="v1.3.0";
inline Color title_color(std::size_t index) {

    return index<2?palette::brand_red:index<7?palette::brand_green:palette::title_neutral;
}
inline float draw_title(Canvas& c,float x,float y,float scale,TitleLayout* out=nullptr) {
    const float advance=c.advance(scale),bold=std::max(1.f,std::round(scale*.6f));
    const float split=x+2*advance+advance*.5f;
    for(std::size_t i=0;title_text[i];++i) {
        const float gx=x+float(i)*advance;
        for(const float offset:{0.f,bold}) {
            if(i==2){
                c.glyph('D',gx+offset,y,scale,palette::brand_red,false,-std::numeric_limits<float>::infinity(),split);
                c.glyph('D',gx+offset,y,scale,palette::brand_green,false,split,std::numeric_limits<float>::infinity());
            }else c.glyph(title_text[i],gx+offset,y,scale,title_color(i));
        }
    }
    if(out)*out={x,y,scale,bold,split};
    return (sizeof(title_text)-1)*advance+bold;
}

struct Frame {
    std::vector<Row> rows;
    unsigned selected=0;
    std::string status;
    std::string fps;
    std::string fps_detail;
    std::string hints;
    std::string position;
    std::string empty="-";
    std::string fallback_status;
    bool show_help=true;
};
inline constexpr unsigned panel_columns=52;
inline std::string footer_text(const Frame& f){return f.position.empty()?f.hints:f.position+"   "+f.hints;}
struct Line { int row; std::string group; };
struct Geometry {
    float x,y,width,height,scale,pad;
    float title_y,fps_y,status_y,list_y,list_height,help_y,footer_y;
    float row_height,header_height,row_scale,small_scale,title_scale,group_scale;
    unsigned status_lines,help_lines,footer_lines,label_columns,value_columns,small_columns;
    std::vector<Line> lines;
    unsigned first,count;
    unsigned first_row,visible_rows;
    int selected_line;
};
inline float line_height(const Geometry& g,const Line& l){return l.row<0?g.header_height:g.row_height;}
inline std::vector<Line> build_lines(const std::vector<Row>& rows) {
    std::vector<Line> lines;lines.reserve(rows.size()*2);
    std::string last;bool first=true;
    for(std::size_t i=0;i<rows.size();++i){
        if(!rows[i].group.empty()&&(first||rows[i].group!=last))lines.push_back({-1,rows[i].group});
        last=rows[i].group;first=false;lines.push_back({int(i),rows[i].group});
    }
    return lines;
}

inline Geometry geometry(unsigned width,unsigned height,const Frame& f,float cell_width=10,float cell_height=20) {
    Geometry g{};
    const float W=float(width),H=float(height);

    const float s=std::clamp(H/1080.f,.8f,1.5f)*std::min(1.f,H/420.f);
    g.scale=s;g.pad=10*s;
    g.title_scale=1.0f*s;g.row_scale=.8f*s;g.small_scale=.68f*s;g.group_scale=.62f*s;
    g.row_height=cell_height*g.row_scale+5*s;g.header_height=cell_height*g.group_scale+9*s;
    const float margin=std::min(16*s,std::min(W,H)/24.f);
    const float cw=cell_width*g.row_scale,sw=cell_width*g.small_scale;

    const float scrollbar=6*s;
    const float max_width=std::max(300*s,W*.42f);
    g.width=std::min(std::min(float(panel_columns)*cw+2*g.pad+scrollbar,max_width),W-2*margin);
    g.x=W-margin-g.width;g.y=margin;
    const float inner=g.width-2*g.pad-scrollbar;
    g.small_columns=std::max(1u,unsigned((g.width-2*g.pad)/sw));
    g.value_columns=0;for(const auto& r:f.rows)g.value_columns=std::max<unsigned>(g.value_columns,unsigned(columns_of(r.value)));
    g.value_columns=std::min(g.value_columns,unsigned(inner/cw/2));
    g.label_columns=std::max(1u,unsigned(inner/cw)-std::min(unsigned(inner/cw)-1,g.value_columns+2));

    const float small_line=cell_height*g.small_scale+3*s;
    g.title_y=g.y+g.pad;
    g.fps_y=g.title_y+cell_height*g.title_scale+4*s;
    g.status_y=g.fps_y+small_line;
    const std::string& status=f.status.empty()?f.fallback_status:f.status;

    const bool healthy=network_ms_span(status).end>0;
    g.status_lines=healthy?1u:std::max(1u,Canvas::wrapped_lines(status,g.small_columns,2));
    const float header=g.status_y+g.status_lines*small_line+6*s-g.y;

    g.help_lines=f.show_help?4:0;
    g.footer_lines=std::max(1u,Canvas::wrapped_lines(footer_text(f),g.small_columns,2));
    const float help_block=g.help_lines?g.help_lines*small_line+6*s:0.f;
    const float footer=8*s+help_block+g.footer_lines*small_line+g.pad;
    g.lines=build_lines(f.rows);
    float content=0;for(const auto& l:g.lines)content+=line_height(g,l);
    const float available=std::max(g.row_height,H-2*margin-header-footer);
    const float cap=std::min(available,18*g.row_height);
    g.list_height=std::min(content,cap);
    if(f.rows.empty())g.list_height=g.row_height;
    g.list_y=g.y+header;
    g.help_y=g.list_y+g.list_height+8*s;
    g.footer_y=g.help_y+help_block;
    g.height=std::min(H-2*margin,header+g.list_height+footer);

    const unsigned selected=f.rows.empty()?0:std::min<unsigned>(f.selected,unsigned(f.rows.size()-1));
    g.selected_line=-1;for(std::size_t i=0;i<g.lines.size();++i)if(g.lines[i].row==int(selected)){g.selected_line=int(i);break;}
    g.first=0;g.count=0;
    if(!g.lines.empty()){
        const unsigned sel=unsigned(std::max(0,g.selected_line));
        unsigned first=sel,last=sel;float used=line_height(g,g.lines[sel]);

        while(first>0&&used+line_height(g,g.lines[first-1])<=g.list_height*.5f+.01f){--first;used+=line_height(g,g.lines[first]);}
        while(last+1<g.lines.size()&&used+line_height(g,g.lines[last+1])<=g.list_height+.01f){++last;used+=line_height(g,g.lines[last]);}
        while(first>0&&used+line_height(g,g.lines[first-1])<=g.list_height+.01f){--first;used+=line_height(g,g.lines[first]);}

        if(last>first&&g.lines[last].row<0){used-=line_height(g,g.lines[last]);--last;}
        g.first=first;g.count=last-first+1;
    }
    g.first_row=0;g.visible_rows=0;bool any=false;
    for(unsigned i=g.first;i<g.first+g.count;++i)if(g.lines[i].row>=0){if(!any){g.first_row=unsigned(g.lines[i].row);any=true;}++g.visible_rows;}
    return g;
}

inline std::string pad_digitos(const std::string& t,std::size_t n) {
    std::size_t k=0;while(k<t.size()&&t[k]>='0'&&t[k]<='9')++k;
    return k==0||k>=n?t:std::string(n-k,' ')+t;
}
inline Canvas build_compacto(unsigned width,unsigned height,const Frame& f,float atlas_width=160,float atlas_height=120,
                             float cell_width=10,float cell_height=20,int rede_ligada=-1) {
    Canvas c;c.atlas_width=atlas_width;c.atlas_height=atlas_height;c.cell_width=cell_width;c.cell_height=cell_height;
    if(width<320||height<300)return c;
    const float H=float(height),W=float(width);
    const float s=std::clamp(H/1080.f,.8f,1.5f)*std::min(1.f,H/420.f);
    const float esc=.8f*s,pad=6*s,margin=std::min(16*s,std::min(W,H)/24.f);
    const std::string& status=f.status.empty()?f.fallback_status:f.status;
    const auto ms=network_ms_span(status);
    std::string rede=ms.end>ms.begin?status.substr(ms.begin,ms.end-ms.begin):std::string("--");

    const std::string fps=pad_digitos(f.fps,3);rede=pad_digitos(rede,3);
    const std::string linha=fps+"  \xc2\xb7  rede "+rede;
    const std::size_t ini=fps.size()+std::string("  \xc2\xb7  rede ").size();
    const float largura=float(columns_of(linha))*cell_width*esc+2*pad,altura=cell_height*esc+2*pad;
    const float x=W-margin-largura,y=margin;
    c.rect(x,y,largura,altura,palette::background);
    const float strip=std::max(1.f,std::round(2*s));
    c.rect(x,y,largura*.5f,strip,palette::brand_red);c.rect(x+largura*.5f,y,largura*.5f,strip,palette::brand_green);
    c.text(linha,x+pad,y+pad,esc,palette::fps,unsigned(columns_of(linha))+1,1,ms.end>ms.begin?ini:0,ms.end>ms.begin?linha.size():0,network_ms_color(ms.ms));
    if(rede_ligada>=0){
        const std::string selo=rede_ligada?"DLSS 5 ON":"DLSS 5 OFF";
        constexpr Color verde{.13f,.55f,.13f,.95f},vermelho{.80f,.10f,.12f,.95f},branco{1,1,1,1};
        const float bl=float(columns_of(selo))*cell_width*esc+2*pad,ba=cell_height*esc+2*pad*.7f;
        const float bx=W-margin-bl,by=y+altura+3*s;
        c.rect(bx,by,bl,ba,rede_ligada?verde:vermelho);
        c.text(selo,bx+pad,by+pad*.7f,esc,branco,unsigned(columns_of(selo))+1);
    }
    return c;
}
inline Canvas build(unsigned width,unsigned height,const Frame& f,
                    float atlas_width=160,float atlas_height=120,float cell_width=10,float cell_height=20,
                    Geometry* geometry_out=nullptr) {
    Canvas c;c.atlas_width=atlas_width;c.atlas_height=atlas_height;c.cell_width=cell_width;c.cell_height=cell_height;

    if(width<320||height<300)return c;
    const auto g=geometry(width,height,f,cell_width,cell_height);
    if(geometry_out)*geometry_out=g;
    using namespace palette;
    const float s=g.scale,tx=g.x+g.pad;

    c.rect(g.x,g.y,g.width,g.height,background);
    const float strip=std::max(1.f,std::round(2*s));
    c.rect(g.x,g.y,g.width*.5f,strip,brand_red);c.rect(g.x+g.width*.5f,g.y,g.width*.5f,strip,brand_green);
    const float title_width=draw_title(c,tx,g.title_y,g.title_scale);

    c.text(mod_version,tx+title_width+c.advance(g.title_scale)*.6f,g.title_y+(g.title_scale-g.small_scale)*cell_height,g.small_scale,muted,8);

    const auto small_cols=g.small_columns;
    {
        const std::string line=f.fps_detail.empty()?f.fps:f.fps+"   "+f.fps_detail;
        c.text(line,tx,g.fps_y,g.small_scale,muted,small_cols,1,0,f.fps.size(),palette::fps);
    }
    const std::string& status=f.status.empty()?f.fallback_status:f.status;
    const auto ms=network_ms_span(status);
    c.text(status,tx,g.status_y,g.small_scale,faint,small_cols,g.status_lines,ms.begin,ms.end,network_ms_color(ms.ms));
    c.rect(tx,g.list_y-4*s,g.width-2*g.pad,std::max(1.f,s),hairline);

    const float cw=cell_width*g.row_scale;
    const float scrollbar=6*s;
    const float list_right=g.x+g.width-g.pad-scrollbar;
    const unsigned selected=f.rows.empty()?0:std::min<unsigned>(f.selected,unsigned(f.rows.size()-1));
    if(f.rows.empty())c.text(f.empty,tx,g.list_y+2*s,g.row_scale,muted,g.label_columns+g.value_columns);
    float y=g.list_y;
    for(unsigned i=g.first;i<g.first+g.count;++i){
        const auto& l=g.lines[i];
        if(l.row<0){
            const float gy=y+6*s;
            const std::string name=fit(l.group,std::size_t((list_right-tx)/(cell_width*g.group_scale))-2);
            c.text(name,tx,gy,g.group_scale,faint,unsigned(columns_of(name)));
            const float after=tx+float(columns_of(name)+1)*cell_width*g.group_scale;
            if(after<list_right)c.rect(after,gy+cell_height*g.group_scale*.55f,list_right-after,std::max(1.f,s*.75f),hairline);
            y+=g.header_height;continue;
        }
        const auto& row=f.rows[unsigned(l.row)];
        const bool sel=unsigned(l.row)==selected;
        const float ty=y+2.5f*s;
        if(sel){
            c.rect(g.x+g.pad*.5f,y,list_right-g.x-g.pad*.5f+2*s,g.row_height,selection);
            c.rect(g.x+g.pad*.5f,y,std::max(2.f,2.5f*s),g.row_height,selected_value);
        }
        const std::string shown_value=fit(row.value,g.value_columns?g.value_columns:1);
        const float vx=list_right-float(columns_of(shown_value))*cw;
        const auto label_cols=std::size_t(std::max(1.f,(vx-tx)/cw-1));
        const std::string label=fit(row.label,label_cols);
        c.text(label,tx,ty,g.row_scale,sel?selected_text:text,unsigned(label_cols));
        c.text(shown_value,vx,ty,g.row_scale,sel?selected_value:palette::value,unsigned(columns_of(shown_value)));
        y+=g.row_height;
    }

    if(g.count<g.lines.size()&&!g.lines.empty()){
        const float bx=g.x+g.width-g.pad*.5f-3*s,bw=std::max(2.f,2*s);
        c.rect(bx,g.list_y,bw,g.list_height,track);
        const float total=float(g.lines.size());
        c.rect(bx,g.list_y+g.list_height*float(g.first)/total,bw,std::max(4*s,g.list_height*float(g.count)/total),thumb);
    }
    c.rect(tx,g.help_y-4*s,g.width-2*g.pad,std::max(1.f,s),hairline);
    if(g.help_lines&&!f.rows.empty())c.text(f.rows[selected].help,tx,g.help_y,g.small_scale,muted,small_cols,g.help_lines);
    c.text(footer_text(f),tx,g.footer_y,g.small_scale,faint,small_cols,g.footer_lines,0,f.position.size(),palette::value);
    return c;
}
}
