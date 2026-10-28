// Windows host. The simulation has no dependency on this file or Windows APIs.
#include <windows.h>
#include <gdiplus.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "game.hpp"

using namespace Gdiplus;
namespace fs = std::filesystem;
namespace {
bloons::Game game;
bloons::Input input;
enum class Screen { Menu, Instructions, Levels, Game };
Screen screen=Screen::Menu;
bool muted=false, mouseHeld=false;
HWND windowHandle;
fs::path root;
std::map<std::wstring,std::unique_ptr<Bitmap>> images;
std::map<int,std::wstring> spriteNames;
std::vector<std::wstring> audioAliases;
std::unique_ptr<PrivateFontCollection> originalFonts;
int soundId=0;
double accumulator=0;
ULONGLONG previousTime=0;
std::uint64_t backgroundFrame=0;

fs::path profilePath() {
    wchar_t path[MAX_PATH]{};
    if(GetEnvironmentVariableW(L"LOCALAPPDATA",path,MAX_PATH)==0) return L"bloons.profile";
    fs::path dir=fs::path(path)/L"Bloons";
    fs::create_directories(dir);
    return dir/L"profile.txt";
}
void save() {
    std::ofstream out(profilePath());
    out<<"BLOONS1\n"<<game.profile.unlocked<<'\n';
    for(double v:game.profile.best) out<<v<<'\n';
}
void load() {
    std::ifstream in(profilePath()); std::string version;
    if(!(in>>version)||version!="BLOONS1") return;
    bloons::Profile p;
    if(!(in>>p.unlocked)||p.unlocked<1||p.unlocked>50) return;
    for(double& v:p.best) if(!(in>>v)||!std::isfinite(v)||v<0||v>100) return;
    game.profile=p;
}
Bitmap* bitmap(const std::wstring& path) {
    auto found=images.find(path); if(found!=images.end()) return found->second.get();
    auto b=std::make_unique<Bitmap>((root/path).c_str());
    if(b->GetLastStatus()!=Ok) return nullptr;
    auto ptr=b.get(); images.emplace(path,std::move(b)); return ptr;
}
void image(Graphics& g,const std::wstring& path,float x,float y,float w,float h) {
    if(auto b=bitmap(path)) g.DrawImage(b,RectF(x,y,w,h));
}
void region(Graphics& g,const std::wstring& path,float x,float y,float w,float h,float sx,float sy,float sw,float sh) {
    if(auto b=bitmap(path)) g.DrawImage(b,RectF(x,y,w,h),sx,sy,sw,sh,UnitPixel);
}
std::wstring spritePath(const std::wstring& name,int frame) {
    return L"sprites/DefineSprite_"+name+L"/"+std::to_wstring(frame)+L".png";
}
void apply(Graphics& g,const bloons::swf::Affine& t) {
    Matrix matrix(REAL(t.a),REAL(t.b),REAL(t.c),REAL(t.d),REAL(t.tx),REAL(t.ty));
    g.MultiplyTransform(&matrix,MatrixOrderPrepend);
}
void originalSprite(Graphics& g,int id,int frame=1,float alpha=1) {
    auto found=spriteNames.find(id); if(found==spriteNames.end()) return;
    auto b=bitmap(spritePath(found->second,frame)); if(!b) return;
    auto r=bloons::swf::spriteBounds(id);
    ColorMatrix cm={1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,alpha,0, 0,0,0,0,1};
    ImageAttributes attrs; attrs.SetColorMatrix(&cm);
    g.DrawImage(b,RectF(float(r.x),float(r.y),float(b->GetWidth()),float(b->GetHeight())),
                0,0,float(b->GetWidth()),float(b->GetHeight()),UnitPixel,&attrs);
}
void text(Graphics& g,const std::wstring& value,float x,float y,float size=16,Color color=Color::Black) {
    FontFamily family;
    INT count=0;
    if(originalFonts) originalFonts->GetFamilies(1,&family,&count);
    Font font(count?&family:FontFamily::GenericSansSerif(),size,FontStyleRegular,UnitPixel);
    SolidBrush brush(color); g.DrawString(value.c_str(),-1,&font,PointF(x,y),&brush);
}
void field(Graphics& g,const std::wstring& value,float x,float y,float w,float h,float size) {
    FontFamily family; INT count=0;
    if(originalFonts) originalFonts->GetFamilies(1,&family,&count);
    Font font(count?&family:FontFamily::GenericSansSerif(),size,FontStyleRegular,UnitPixel);
    StringFormat format; format.SetAlignment(StringAlignmentFar);
    SolidBrush brush(Color(51,51,51));
    g.DrawString(value.c_str(),-1,&font,RectF(x,y,w,h),&format,&brush);
}
std::wstring widen(const std::string& s) { return std::wstring(s.begin(),s.end()); }
void stopAudio() {
    for(auto& alias:audioAliases) mciSendStringW((L"close "+alias).c_str(),nullptr,0,nullptr);
    audioAliases.clear();
}
void playAudio(bloons::Sound sound) {
    if(sound==bloons::Sound::StopAll) { stopAudio(); return; }
    if(muted) return;
    std::wstring file;
    switch(sound) {
    case bloons::Sound::Bomb: file=L"1_bomb.mp3"; break;
    case bloons::Sound::Ice: file=L"2_ice.mp3"; break;
    case bloons::Sound::Metal: file=L"3_metalClink.mp3"; break;
    case bloons::Sound::Clink: file=L"4_clink.mp3"; break;
    default: file=std::to_wstring(static_cast<int>(sound))+L".mp3"; break;
    }
    if(audioAliases.size()>24) { mciSendStringW((L"close "+audioAliases.front()).c_str(),nullptr,0,nullptr); audioAliases.erase(audioAliases.begin()); }
    std::wstring alias=L"bloons"+std::to_wstring(soundId++);
    std::wstring command=L"open \""+(root/L"sounds"/file).wstring()+L"\" type mpegvideo alias "+alias;
    if(mciSendStringW(command.c_str(),nullptr,0,nullptr)==0) {
        mciSendStringW((L"play "+alias).c_str(),nullptr,0,nullptr); audioAliases.push_back(alias);
    }
}
void startLevel(int number) { game.start(number); screen=Screen::Game; accumulator=0; }
void drawBalloon(Graphics& g,const bloons::Balloon& b) {
    auto state=g.Save(); g.TranslateTransform(float(b.position.x),float(b.position.y));
    int id=b.character();
    if(b.popped) {
        if(id==63) { if(b.popFrame<12) originalSprite(g,id,2+b.popFrame); }
        else if(b.popFrame<=4) originalSprite(g,id,98+b.popFrame);
    } else if(auto style=bloons::swf::balloonStyle(id)) {
        // Compose the stopped inner clip with the actual chosen outer-frame
        // matrix. Flattened exported idle frames erroneously advance freeze.
        apply(g,style->poses[b.animation%80].inner);
        originalSprite(g,style->body);
        if(b.frozen) {
            g.TranslateTransform(.05f,-.05f,MatrixOrderPrepend);
            originalSprite(g,49);
        }
    } else originalSprite(g,id);
    g.Restore(state);
}
void drawBackground(Graphics& g) {
    // Recreate shape 183's gradient using its exported SVG transform/stops.
    LinearGradientBrush sky(PointF(0,-82.0728f),PointF(0,547.0728f),Color(0,153,255),Color::White);
    g.FillRectangle(&sky,0,0,640,480);
    auto state=g.Save();
    apply(g,bloons::swf::cloudPoses[backgroundFrame%bloons::swf::cloudPoses.size()]);
    originalSprite(g,186);
    g.Restore(state);
}
void drawProjectile(Graphics& g,const bloons::Projectile& p,int id) {
    auto state=g.Save(); g.TranslateTransform(float(p.position.x),float(p.position.y));
    g.RotateTransform(float(p.rotation));
    originalSprite(g,id,1,float(p.alpha/100)); g.Restore(state);
}
void drawDart(Graphics& g,const bloons::Projectile& d) {
    auto state=g.Save();
    g.TranslateTransform(float(d.position.x),float(d.position.y));
    g.RotateTransform(float(d.rotation));
    auto scale=d.extra?bloons::swf::extraDart:bloons::swf::dart;
    g.ScaleTransform(float(scale.a),float(scale.d));
    if(d.boomerang) {
        apply(g,bloons::swf::boomerangArt);
        originalSprite(g,37,1+(d.age%20),float(d.alpha/100));
    } else {
        g.TranslateTransform(-55.8f,-251.65f,MatrixOrderPrepend);
        originalSprite(g,33,1,float(d.alpha/100));
    }
    g.Restore(state);
}
void drawGame(Graphics& g) {
    drawBackground(g);
    if(game.level->number==50) originalSprite(g,622);
    auto state=g.Save(); g.TranslateTransform(0,float(game.shake));
    for(auto& b:game.balloons) drawBalloon(g,b);
    {
        auto monkey=g.Save(); g.TranslateTransform(float(game.monkey.x),float(game.monkey.y));
        originalSprite(g,155);
        auto arm=g.Save();
        if(game.throwFrame>0) {
            apply(g,bloons::swf::armThrow);
            apply(g,bloons::swf::throwPoses[std::min(8,game.throwFrame-1)]);
        } else {
            apply(g,bloons::swf::monkeyArm);
            g.RotateTransform(float(game.aimAngle*57.29577951308232));
        }
        originalSprite(g,152); g.Restore(arm); g.Restore(monkey);
    }
    for(auto& t:game.tacks) if(!t.dead) drawProjectile(g,t,12);
    for(auto s:game.spikeys) if(!s.dead) { s.rotation*=s.age; drawProjectile(g,s,8); }
    if(game.pacmanActive) {
        auto state=g.Save(); g.TranslateTransform(float(game.pacman.x),float(game.pacman.y));
        originalSprite(g,163,1+game.pacmanAge%8); g.Restore(state);
    }
    for(auto& d:game.darts) if(d.launched||d.aiming) drawDart(g,d);
    if(game.active&&!game.darts[0].launched&&!game.pacmanActive) {
        auto arrow=g.Save(); g.TranslateTransform(float(game.aimTip.x),float(game.aimTip.y));
        g.RotateTransform(float(game.aimAngle*57.29577951308232));
        apply(g,bloons::swf::aimInner);
        g.ScaleTransform(1,game.darts[0].aiming?float(1+(game.power-18)*.08):1);
        originalSprite(g,170,game.darts[0].aiming?2:1);
        g.Restore(arrow);
    }
    g.Restore(state);
    // The root, wrapper, and stopped UI frame each have their own placement.
    auto uiState=g.Save(); apply(g,bloons::swf::gameui); apply(g,bloons::swf::ui);
    originalSprite(g,304);
    // Hide the baked-in full dart rack and power-up icons; draw the live state.
    auto ub=bloons::swf::spriteBounds(304);
    region(g,spritePath(L"304",1),23,20,110,37,145,float(20-ub.y),1,37);
    region(g,spritePath(L"304",1),177,20,78,35,145,float(20-ub.y),1,35);
    field(g,std::to_wstring(game.level->number)+L". "+widen(game.level->title),-3.9f,-3.1f,178.15f,31.35f,18);
    field(g,std::to_wstring(game.popped),338.8f,2.65f,63.05f,31.35f,22);
    field(g,std::to_wstring(game.level->target),338.8f,27.25f,63.05f,31.35f,22);
    region(g,spritePath(L"304",1),465,35,51,23,145,float(35-ub.y),1,23);
    field(g,std::to_wstring(int(game.score)),466.55f,34.9f,47.8f,21.4f,14);
    int remaining=game.easy?9:std::max(0,game.level->darts-game.shots);
    for(int i=0;i<remaining&&i<9;++i) {
        auto state=g.Save(); g.TranslateTransform(28.15f+i*11.85f,35.55f);
        g.ScaleTransform(.3109283447265625f,.311126708984375f); originalSprite(g,303); g.Restore(state);
    }
    if(game.gotBoomerang) { auto s=g.Save(); g.TranslateTransform(178.1f,22.5f); originalSprite(g,275); g.Restore(s); }
    if(game.gotMultishot) { auto s=g.Save(); g.TranslateTransform(219.25f,22.85f); originalSprite(g,271); g.Restore(s); }
    g.Restore(uiState);
    if(game.pacmanActive) { auto s=g.Save(); apply(g,bloons::swf::arrowKeys); originalSprite(g,652); g.Restore(s); }
    if(game.outcome==bloons::Outcome::Complete) image(g,L"frames/55.png",0,0,640,480);
    else if(game.outcome==bloons::Outcome::Lost) {
        auto state=g.Save(); apply(g,bloons::swf::losePanel); apply(g,bloons::swf::loseText);
        originalSprite(g,628); g.Restore(state);
    }
    else if(game.outcome==bloons::Outcome::Won) {
        region(g,spritePath(L"650",11),80,130,484,256,986,728,484,256);
        SolidBrush white(Color::White); g.FillRectangle(&white,180,245,260,40);
        double percent=std::round(double(game.popped)/game.total*1000)/10;
        std::wostringstream result; result<<L"You Popped: "<<percent<<L"%";
        text(g,result.str(),190,247,26);
    }
}
void draw(Graphics& g) {
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    if(screen==Screen::Menu) image(g,L"frames/36.png",0,0,640,480);
    else if(screen==Screen::Instructions) image(g,L"frames/47.png",0,0,640,480);
    else if(screen==Screen::Levels) {
        image(g,L"frames/54.png",0,0,640,480);
        // Restore unlocked thumbnails using the original colour export.
        for(auto button:bloons::swf::levelButtons) if(game.easy||button.number<=game.profile.unlocked) {
            auto state=g.Save(); apply(g,button.placement); originalSprite(g,button.sprite,2); g.Restore(state);
        }
        if(game.easy) { auto state=g.Save(); apply(g,bloons::swf::unlimitedDarts); originalSprite(g,611,2); g.Restore(state); }
    } else drawGame(g);
}
bool inside(int x,int y,int w,int h) { return bloons::Rect{double(x),double(y),double(w),double(h)}.contains(input.mouse); }
void click() {
    if(screen==Screen::Menu) {
        if(inside(145,35,340,70)) { game.score=0; screen=Screen::Levels; }
        else if(inside(145,195,340,70)) ShellExecuteW(nullptr,L"open",L"http://ninjakiwi.com/Games/Bloons-Games/Bloons2.html",nullptr,nullptr,SW_SHOWNORMAL);
        else if(inside(145,275,340,70)) ShellExecuteW(nullptr,L"open",L"http://www.ninjakiwi.com",nullptr,nullptr,SW_SHOWNORMAL);
        else if(inside(145,355,460,70)) ShellExecuteW(nullptr,L"open",L"http://www.myninjakiwi.com",nullptr,nullptr,SW_SHOWNORMAL);
        // The Mochi leaderboard requires its Flash SDK and has no native adapter.
        // Keep its original artwork; do not fabricate a replacement leaderboard.
    } else if(screen==Screen::Instructions) screen=Screen::Menu;
    else if(screen==Screen::Levels) {
        if(inside(555,385,30,30)) game.easy=!game.easy;
        else if(inside(250,418,110,62)) screen=Screen::Menu;
        else for(auto button:bloons::swf::levelButtons) {
            auto r=button.hit;
            if(bloons::Rect{r.x,r.y,r.w,r.h}.contains(input.mouse)&&(game.easy||button.number<=game.profile.unlocked)) { startLevel(button.number); break; }
        }
    } else {
        if(inside(525,457,115,23)) { ShellExecuteW(nullptr,L"open",L"http://www.ninjakiwi.com",nullptr,nullptr,SW_SHOWNORMAL); return; }
        if(inside(525,424,115,33)) { screen=Screen::Menu; stopAudio(); return; }
        if(inside(400,424,65,50)) { game.score-=game.levelScore; startLevel(game.level->number); return; }
        if(game.outcome==bloons::Outcome::Complete) { screen=Screen::Menu; return; }
        if(game.outcome==bloons::Outcome::Won) {
            if(inside(320,285,150,70)) { game.score-=game.levelScore; startLevel(game.level->number); }
            else if(inside(175,285,140,70)) {
                startLevel(game.level->number+1);
            }
        } else if(input.mouse.y<415) { game.input=input; game.press(); mouseHeld=true; }
    }
}
void mousePosition(LPARAM param) {
    RECT r; GetClientRect(windowHandle,&r);
    double scale=std::min(r.right/640.0,r.bottom/480.0);
    input.mouse={((short)LOWORD(param)-(r.right-640*scale)/2)/scale,((short)HIWORD(param)-(r.bottom-480*scale)/2)/scale};
}
LRESULT CALLBACK windowProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN: mousePosition(lp); SetCapture(hwnd); click(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:
        mousePosition(lp); ReleaseCapture();
        if(mouseHeld&&screen==Screen::Game) { game.input=input; game.release(); }
        mouseHeld=false; return 0;
    case WM_MOUSEMOVE: mousePosition(lp); return 0;
    case WM_KILLFOCUS:
        input.up=input.down=input.left=input.right=false;
        if(mouseHeld) { game.darts[0].aiming=false; mouseHeld=false; } return 0;
    case WM_KEYDOWN: case WM_KEYUP: {
        bool down=message==WM_KEYDOWN;
        if(wp==VK_UP) input.up=down; if(wp==VK_DOWN) input.down=down;
        if(wp==VK_LEFT) input.left=down; if(wp==VK_RIGHT) input.right=down;
        return 0;
    }
    case WM_TIMER: {
        auto now=GetTickCount64(); accumulator+=double(now-previousTime); previousTime=now;
        if(screen==Screen::Game) {
            // Discard time spent suspended instead of advancing the game offscreen.
            accumulator=std::min(accumulator,250.0);
            while(accumulator>=bloons::frameMilliseconds) {
                auto old=game.outcome; game.tick(input); accumulator-=bloons::frameMilliseconds;
                ++backgroundFrame;
                for(auto s:game.sounds) playAudio(s); game.sounds.clear();
                if(old!=game.outcome) save();
            }
        } else accumulator=0;
        InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);
        RECT r; GetClientRect(hwnd,&r);
        if(r.right>0&&r.bottom>0) {
            Bitmap back(r.right,r.bottom,PixelFormat32bppPARGB); Graphics g(&back); g.Clear(Color::Black);
            float scale=std::min(r.right/640.f,r.bottom/480.f);
            g.TranslateTransform((r.right-640*scale)/2,(r.bottom-480*scale)/2); g.ScaleTransform(scale,scale);
            draw(g); Graphics screenGraphics(dc); screenGraphics.DrawImage(&back,0,0);
        }
        EndPaint(hwnd,&ps); return 0;
    }
    case WM_DESTROY: save(); stopAudio(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR commandLine,int show) {
    GdiplusStartupInput startup; ULONG_PTR token;
    if(GdiplusStartup(&token,&startup,nullptr)!=Ok) return 1;
    wchar_t executable[MAX_PATH]; GetModuleFileNameW(nullptr,executable,MAX_PATH);
    root=fs::path(executable).parent_path();
    if(!fs::exists(root/L"frames/36.png")) root=BLOONS_ASSET_ROOT;
    if(!fs::exists(root/L"frames/36.png")) {
        MessageBoxW(nullptr,L"Place the original frames, sprites and sounds folders beside bloons.exe.",L"Missing original assets",MB_ICONERROR); return 1;
    }
    originalFonts=std::make_unique<PrivateFontCollection>();
    originalFonts->AddFontFile((root/L"fonts/175_SoupBone-Bold.ttf").c_str());
    for(auto& entry:fs::directory_iterator(root/L"sprites")) {
        auto name=entry.path().filename().wstring();
        if(name.rfind(L"DefineSprite_",0)==0) {
            auto suffix=name.substr(13); spriteNames.emplace(std::stoi(suffix),suffix);
        }
    }
    // Developer-only render check; no window, profile access, input or audio.
    if(std::wstring(commandLine)==L"--render-check") {
        CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
        fs::create_directories(root/L"build/render");
        int failures=0;
        for(int scene=0;scene<11;++scene) {
            Bitmap result(640,480,PixelFormat32bppARGB);
            {
                Graphics graphics(&result);
                if(scene==0) screen=Screen::Menu;
                else if(scene==1) screen=Screen::Levels;
                else if(scene==2) screen=Screen::Instructions;
                else {
                    startLevel(scene==4?25:scene==5?50:1);
                    input.mouse={500,100};
                    if(scene==6||scene==7) {
                        game.press(); for(int i=0;i<12;++i) game.tick(input);
                        if(scene==7) { game.release(); for(int i=0;i<5;++i) game.tick(input); }
                    }
                    if(scene==8) for(auto& b:game.balloons) b.frozen=true;
                    if(scene==9) {
                        for(std::size_t i=0;i<game.balloons.size();++i) game.pop(i);
                        for(int i=0;i<51;++i) game.tick(input);
                    }
                    if(scene==10) { game.shots=game.level->darts; for(int i=0;i<51;++i) game.tick(input); }
                }
                draw(graphics);
            }
            auto path=root/L"build/render"/(std::to_wstring(scene)+L".png");
            if(result.Save(path.c_str(),&png)!=Ok) ++failures;
        }
        screen=Screen::Menu; input.mouse={300,70}; click();
        if(screen!=Screen::Levels) ++failures;
        input.mouse={50,110}; click();
        if(screen!=Screen::Game||!game.level||game.level->number!=1) ++failures;
        input.mouse={500,150}; click(); game.tick(input); game.release();
        if(!game.darts[0].launched) ++failures;
        images.clear(); originalFonts.reset(); GdiplusShutdown(token); return failures;
    }
    load();
    WNDCLASSW wc{}; wc.lpfnWndProc=windowProc; wc.hInstance=instance; wc.lpszClassName=L"BloonsCpp"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClassW(&wc); RECT area{0,0,640,480}; AdjustWindowRect(&area,WS_OVERLAPPEDWINDOW,FALSE);
    windowHandle=CreateWindowW(wc.lpszClassName,L"Bloons",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,area.right-area.left,area.bottom-area.top,nullptr,nullptr,instance,nullptr);
    ShowWindow(windowHandle,show); previousTime=GetTickCount64(); SetTimer(windowHandle,1,8,nullptr);
    MSG msg; while(GetMessageW(&msg,nullptr,0,0)>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    images.clear(); originalFonts.reset(); GdiplusShutdown(token); return 0;
}
