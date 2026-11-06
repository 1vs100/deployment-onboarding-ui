#include "ui/text_atlas.h"

#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/imgui/imstb_truetype.h"

namespace onboarding::ui {
namespace {

constexpr int kAtlasWidth = 1024;
constexpr int kAtlasHeight = 512;
constexpr int kFirstChar = 32;
constexpr int kCharCount = 95;
constexpr int kSizes = 3;

}  // namespace

struct TextAtlas::Impl {
  stbtt_fontinfo font{};
  float px[kSizes] = {kTitlePx, kBodyPx, kSmallPx};
  float ascent[kSizes] = {};
  stbtt_packedchar chars[kSizes][kCharCount] = {};
  std::vector<unsigned char> coverage;
  SDL_Texture* texture = nullptr;

  int Index(TextSize size) const {
    switch (size) {
      case TextSize::kTitle: return 0;
      case TextSize::kBody: return 1;
      case TextSize::kSmall: return 2;
    }
    return 2;
  }

  // Advances `pen_x` by the string and returns its width. stbtt_GetPackedQuad
  // always writes the quad, so measuring needs one to throw away.
  float Advance(int index, std::string_view text) const {
    float pen_x = 0;
    float pen_y = 0;
    stbtt_aligned_quad scratch;
    for (char c : text) {
      const int codepoint = static_cast<unsigned char>(c);
      if (codepoint < kFirstChar || codepoint >= kFirstChar + kCharCount) continue;
      stbtt_GetPackedQuad(chars[index], kAtlasWidth, kAtlasHeight, codepoint - kFirstChar, &pen_x,
                          &pen_y, &scratch, 0);
    }
    return pen_x;
  }
};

TextAtlas::TextAtlas() : impl_(std::make_unique<Impl>()) {}

TextAtlas::~TextAtlas() {
  if (impl_ && impl_->texture) SDL_DestroyTexture(impl_->texture);
}

bool TextAtlas::Load(const unsigned char* ttf, size_t size, std::string* error) {
  Impl& impl = *impl_;
  const int offset = stbtt_GetFontOffsetForIndex(ttf, 0);
  if (offset < 0 || !stbtt_InitFont(&impl.font, ttf, offset)) {
    if (error) *error = "cannot parse the embedded font";
    return false;
  }
  impl.coverage.assign(static_cast<size_t>(kAtlasWidth) * kAtlasHeight, 0);

  stbtt_pack_context pack;
  if (!stbtt_PackBegin(&pack, impl.coverage.data(), kAtlasWidth, kAtlasHeight, 0, 2, nullptr)) {
    if (error) *error = "cannot start the glyph packer";
    return false;
  }
  stbtt_PackSetOversampling(&pack, 1, 1);
  stbtt_pack_range ranges[kSizes] = {};
  for (int i = 0; i < kSizes; ++i) {
    ranges[i].font_size = impl.px[i];
    ranges[i].first_unicode_codepoint_in_range = kFirstChar;
    ranges[i].num_chars = kCharCount;
    ranges[i].chardata_for_range = impl.chars[i];
  }
  const bool packed = stbtt_PackFontRanges(&pack, ttf, 0, ranges, kSizes) != 0;
  stbtt_PackEnd(&pack);
  if (!packed) {
    if (error) *error = "the glyph atlas is full";
    return false;
  }
  (void)size;
  int ascent = 0, descent = 0, line_gap = 0;
  stbtt_GetFontVMetrics(&impl.font, &ascent, &descent, &line_gap);
  for (int i = 0; i < kSizes; ++i) {
    impl.ascent[i] = ascent * stbtt_ScaleForPixelHeight(&impl.font, impl.px[i]);
  }
  return true;
}

bool TextAtlas::CreateTexture(SDL_Renderer* renderer, std::string* error) {
  Impl& impl = *impl_;
  std::vector<unsigned char> pixels(static_cast<size_t>(kAtlasWidth) * kAtlasHeight * 4);
  for (size_t i = 0; i < impl.coverage.size(); ++i) {
    pixels[i * 4 + 0] = 255;
    pixels[i * 4 + 1] = 255;
    pixels[i * 4 + 2] = 255;
    pixels[i * 4 + 3] = impl.coverage[i];
  }
  impl.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                   kAtlasWidth, kAtlasHeight);
  if (!impl.texture) {
    if (error) *error = SDL_GetError();
    return false;
  }
  SDL_SetTextureBlendMode(impl.texture, SDL_BLENDMODE_BLEND);
  if (!SDL_UpdateTexture(impl.texture, nullptr, pixels.data(), kAtlasWidth * 4)) {
    if (error) *error = SDL_GetError();
    return false;
  }
  return true;
}

float TextAtlas::Width(TextSize size, std::string_view text) const {
  return impl_->Advance(impl_->Index(size), text);
}

void TextAtlas::Draw(SDL_Renderer* renderer, TextSize size, Color color, const Rect& box,
                     TextAlign align, std::string_view text) const {
  const Impl& impl = *impl_;
  if (!impl.texture || text.empty()) return;
  const int index = impl.Index(size);
  float pen_x = align == TextAlign::kCenter ? box.center_x() - impl.Advance(index, text) / 2 : box.x;
  const float baseline = box.y + (box.h - impl.px[index]) / 2 + impl.ascent[index];

  SDL_SetTextureColorMod(impl.texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(impl.texture, color.a);
  for (char c : text) {
    const int codepoint = static_cast<unsigned char>(c);
    if (codepoint < kFirstChar || codepoint >= kFirstChar + kCharCount) continue;
    stbtt_aligned_quad quad;
    float y = baseline;
    stbtt_GetPackedQuad(impl.chars[index], kAtlasWidth, kAtlasHeight, codepoint - kFirstChar, &pen_x,
                        &y, &quad, 0);
    const SDL_FRect dst{quad.x0, quad.y0, quad.x1 - quad.x0, quad.y1 - quad.y0};
    const SDL_FRect src{
        quad.s0 * kAtlasWidth,
        quad.t0 * kAtlasHeight,
        (quad.s1 - quad.s0) * kAtlasWidth,
        (quad.t1 - quad.t0) * kAtlasHeight,
    };
    SDL_RenderTexture(renderer, impl.texture, &src, &dst);
  }
}

}  // namespace onboarding::ui
