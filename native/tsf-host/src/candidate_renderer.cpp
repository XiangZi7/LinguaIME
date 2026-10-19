#include "candidate_window.h"
#include "diagnostics.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace {
constexpr float kInset = 9.0f;
constexpr float kWidth = 350.0f;
D2D1_COLOR_F color(unsigned rgb, float alpha = 1.0f) {
    return D2D1::ColorF(rgb, alpha);
}
}

bool CandidateWindow::draw(HDC dc, const RECT& bounds) {
    if (!drawing_factory_) {
        auto result=D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,drawing_factory_.GetAddressOf());
        if (FAILED(result)) return false;
        result=DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(text_factory_.GetAddressOf()));
        if (FAILED(result)) return false;
    }
    if (!target_) {
        const auto properties=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(drawing_factory_->CreateDCRenderTarget(&properties,&target_))) return false;
    }
    if (FAILED(target_->BindDC(dc,&bounds))) return false;
    target_->SetDpi(static_cast<float>(scale_),static_cast<float>(scale_));
    target_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    target_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    const bool dark=config_.theme==2;
    const float right=kInset+kWidth;
    const float bottom=static_cast<float>(bounds.bottom)*96.0f/static_cast<float>(scale_)-kInset;
    const auto primary=color(dark?0xF3F5F7:0x202731);
    const auto secondary=color(dark?0xAFBAC7:0x7B8796);
    const auto orange=color(dark?0xF3A16D:0xDC702D);
    ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(target_->CreateSolidColorBrush(primary,&brush))) return false;
    auto fill=[&](D2D1_RECT_F rectangle,float radius,D2D1_COLOR_F value) {
        brush->SetColor(value);
        target_->FillRoundedRectangle(D2D1::RoundedRect(rectangle,radius,radius),brush.Get());
    };
    auto line=[&](float x1,float y1,float x2,float y2,D2D1_COLOR_F value,float width=1.0f) {
        brush->SetColor(value);
        target_->DrawLine(D2D1::Point2F(x1,y1),D2D1::Point2F(x2,y2),brush.Get(),width);
    };
    auto text=[&](const std::wstring& value,D2D1_RECT_F rectangle,float size,D2D1_COLOR_F ink,
                  bool semibold=false,DWRITE_TEXT_ALIGNMENT alignment=DWRITE_TEXT_ALIGNMENT_LEADING) {
        if (value.empty()) return;
        ComPtr<IDWriteTextFormat> format;
        if (FAILED(text_factory_->CreateTextFormat(L"Microsoft YaHei",nullptr,
            semibold?DWRITE_FONT_WEIGHT_SEMI_BOLD:DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,size,L"zh-CN",&format))) return;
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        format->SetTextAlignment(alignment);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER,0,0};
        ComPtr<IDWriteInlineObject> ellipsis;
        text_factory_->CreateEllipsisTrimmingSign(format.Get(),&ellipsis);
        format->SetTrimming(&trimming,ellipsis.Get());
        brush->SetColor(ink);
        target_->DrawTextW(value.c_str(),static_cast<UINT32>(value.size()),format.Get(),rectangle,brush.Get(),D2D1_DRAW_TEXT_OPTIONS_CLIP);
    };
    target_->BeginDraw();
    target_->Clear(D2D1::ColorF(0,0.0f));
    // Premultiplied alpha keeps rounded corners and the shadow independent of DWM/nonclient borders.
    for (int spread=8;spread>0;--spread) {
        const float extent=static_cast<float>(spread);
        fill(D2D1::RectF(kInset-extent,kInset-extent+2,right+extent,bottom+extent+2),12+extent,
             color(0x324963,dark?0.018f:0.010f));
    }
    const auto surface=D2D1::RectF(kInset,kInset,right,bottom);
    fill(surface,12,color(dark?0x20262D:0xFFFFFF));
    brush->SetColor(color(dark?0x3A4652:0xE3E8EE));
    target_->DrawRoundedRectangle(D2D1::RoundedRect(surface,12,12),brush.Get(),1.0f);

    // Composition header: unobtrusive native icon, pinyin, then the display mode.
    const float icon_x=kInset+19,icon_y=kInset+21;
    brush->SetColor(color(dark?0x8CCBD8:0x4F8C9E));
    if (config_.display_mode==0) {
        target_->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(icon_x-6,icon_y-5,icon_x+6,icon_y+5),2,2),brush.Get(),1);
        line(icon_x-3,icon_y+2,icon_x+3,icon_y+2,brush->GetColor());
    } else {
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(icon_x,icon_y),6,6),brush.Get(),1);
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(icon_x,icon_y),2.5f,6),brush.Get(),0.8f);
        line(icon_x-6,icon_y,icon_x+6,icon_y,brush->GetColor(),0.8f);
    }
    text(composition_,D2D1::RectF(kInset+34,kInset+2,right-115,kInset+40),12,primary,true);
    const auto mode=config_.display_mode==0?L"中文模式":config_.display_mode==2?L"双语详细模式":L"双语简洁模式";
    text(mode,D2D1::RectF(right-112,kInset+2,right-15,kInset+40),11,secondary,false,DWRITE_TEXT_ALIGNMENT_TRAILING);
    for (unsigned index=0;index<page_.item_count;++index) {
        const float detail_offset=config_.display_mode==2 && index>selected_?54.0f:0.0f;
        const float top=kInset+42+static_cast<float>(index*row_height_)+detail_offset;
        const float height=static_cast<float>(row_height_);
        const bool selected=index==selected_;
        if (selected) {
            unsigned selected_color=dark?0x28434B:0xE1F4F5;
            if (config_.accent==0) selected_color=dark?0x394936:0xDCEBC8;
            if (config_.accent==1) selected_color=dark?0x2D405C:0xDEEBFC;
            const auto selected_rectangle=D2D1::RectF(kInset+7,top,right-7,top+height-1);
            fill(selected_rectangle,8,color(selected_color));
            brush->SetColor(color(dark?0x3C6570:0xCCEDF0));
            target_->DrawRoundedRectangle(D2D1::RoundedRect(selected_rectangle,8,8),brush.Get(),0.7f);
        }
        const float first_line=height;
        text(std::to_wstring(index+1),D2D1::RectF(kInset+18,top,kInset+35,top+first_line),12,secondary);
        const float chinese_right=config_.display_mode==0?right-18:kInset+139;
        text(wide(page_.items[index].text),D2D1::RectF(kInset+42,top,chinese_right,top+first_line),static_cast<float>(config_.font_size),primary);
        if (config_.display_mode!=0) {
            text(wide(page_.items[index].translation),D2D1::RectF(kInset+151,top,right-15,top+first_line),12,orange);
        }
        if (config_.display_mode==2 && selected) {
            fill(D2D1::RectF(kInset+8,top+height,right-8,top+height+52),0,color(dark?0x252E37:0xF5F8FB));
            text(L"拼音   "+wide(page_.items[index].reading),D2D1::RectF(kInset+42,top+height+3,right-15,top+height+25),11.5f,secondary);
            text(wide(page_.items[index].translation),D2D1::RectF(kInset+42,top+height+25,right-15,top+height+48),11.5f,secondary);
        }
    }
    const float footer=kInset+42+static_cast<float>(page_.item_count*row_height_)+(config_.display_mode==2 && page_.item_count?54.0f:0.0f);
    line(kInset+14,footer+2,right-14,footer+2,color(dark?0x34404C:0xF0F3F6));
    std::wstring indices;
    for (unsigned index=0;index<config_.page_size;++index) { if (index) indices+=L" "; indices+=std::to_wstring(index+1); }
    text(indices,D2D1::RectF(kInset+15,footer+3,kInset+121,bottom-3),10.5f,secondary);
    auto page_key=[](uint32_t key)->std::wstring { return key==0x21?L"PgUp":key==0x22?L"PgDn":key==0xDB?L"[":key==0xDD?L"]":key==0xBD?L"-":L"="; };
    const auto paging=page_key(config_.previous_key)+L" 上一页   "+page_key(config_.next_key)+L" 下一页";
    text(paging,D2D1::RectF(kInset+128,footer+3,right-55,bottom-3),10.5f,secondary);
    const auto number=std::to_wstring(page_.page_count?page_.page_index+1:0)+L"/"+std::to_wstring(page_.page_count);
    text(number,D2D1::RectF(right-50,footer+3,right-15,bottom-3),10.5f,secondary,false,DWRITE_TEXT_ALIGNMENT_TRAILING);
    const auto result=target_->EndDraw();
    if (result==D2DERR_RECREATE_TARGET) target_.Reset();
    if (FAILED(result)) log_error("CANDIDATE_RENDER_FAILED",result);
    return SUCCEEDED(result);
}

void CandidateWindow::paint() {
    if (!window_) return;
    RECT bounds{},position{};
    GetClientRect(window_,&bounds);
    GetWindowRect(window_,&position);
    if (bounds.right<=0 || bounds.bottom<=0) return;
    const HDC screen=GetDC(nullptr);
    const HDC dc=CreateCompatibleDC(screen);
    BITMAPINFO information{};
    information.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    information.bmiHeader.biWidth=bounds.right;
    information.bmiHeader.biHeight=-bounds.bottom;
    information.bmiHeader.biPlanes=1;
    information.bmiHeader.biBitCount=32;
    information.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;
    const HBITMAP bitmap=CreateDIBSection(dc,&information,DIB_RGB_COLORS,&bits,nullptr,0);
    if (bitmap && bits) {
        const auto old=SelectObject(dc,bitmap);
        if (draw(dc,bounds)) {
            POINT destination{position.left,position.top},origin{0,0};
            SIZE size{bounds.right,bounds.bottom};
            BLENDFUNCTION blending{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
            UpdateLayeredWindow(window_,screen,&destination,&size,dc,&origin,0,&blending,ULW_ALPHA);
            bitmap_width_=bounds.right; bitmap_height_=bounds.bottom;
            const auto* first=static_cast<unsigned char*>(bits);
            pixels_.assign(first,first+static_cast<size_t>(bitmap_width_)*bitmap_height_*4);
        }
        SelectObject(dc,old);
        DeleteObject(bitmap);
    }
    DeleteDC(dc); ReleaseDC(nullptr,screen);
}

bool CandidateWindow::save_preview(const wchar_t* path) const {
    if (pixels_.empty()) return false;
    std::ofstream file(std::filesystem::path(path),std::ios::binary);
    file << "P6\n" << bitmap_width_ << ' ' << bitmap_height_ << "\n255\n";
    for (size_t index=0;index<pixels_.size();index+=4) {
        const unsigned alpha=pixels_[index+3];
        for (const int channel : {2,1,0}) {
            const auto value=static_cast<unsigned char>(std::min(255u,pixels_[index+channel]+247u*(255u-alpha)/255u));
            file.put(static_cast<char>(value));
        }
    }
    return file.good();
}
