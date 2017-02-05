#ifndef TRANCE_SRC_TRANCE_DIRECTOR_H
#define TRANCE_SRC_TRANCE_DIRECTOR_H
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#pragma warning(push, 0)
#include <GL/glew.h>
#include <libovr/OVR_CAPI.h>
#include <libovr/OVR_CAPI_GL.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#pragma warning(pop)

namespace trance_pb
{
  class Program;
  class Session;
  class System;
}

class Font;
class Image;
class ThemeBank;
class Visual;
class VisualApiImpl;
class Director
{
public:
  Director(sf::RenderWindow& window, const trance_pb::Session& session,
           const trance_pb::System& system, ThemeBank& themes, const trance_pb::Program& program,
           bool realtime, bool oculus_rift, bool convert_to_yuv);
  ~Director();

  // Called from play_session() in main.cpp.
  void set_program(const trance_pb::Program& program);
  bool update();
  void render() const;
  // Returns screen data only in non-realtime mode.
  const uint8_t* get_screen_data() const;

  const trance_pb::Program& program() const;
  bool vr_enabled() const;

  void render_spiral(float spiral, uint32_t spiral_width, uint32_t spiral_type) const;
  void render_image(const Image& image, float alpha, float zoom_origin, float zoom) const;

  sf::Vector2f text_size(const Font& font, const std::string& text, bool large) const;
  void render_text(const Font& font, const std::string& text, bool large, const sf::Color& colour,
                   float scale, const sf::Vector2f& offset, float zoom_origin, float zoom) const;

private:
  bool init_framebuffer(uint32_t& fbo, uint32_t& fb_tex, uint32_t width, uint32_t height) const;
  bool init_oculus_rift();
  void change_visual(uint32_t length);
  uint32_t view_width() const;
  float far_plane_distance() const;
  float eye_offset() const;

  sf::RenderWindow& _window;
  const trance_pb::Session& _session;
  const trance_pb::System& _system;
  ThemeBank& _themes;
  uint32_t _width;
  uint32_t _height;
  const trance_pb::Program* _program;

  bool _realtime;
  bool _convert_to_yuv;
  uint32_t _render_fbo;
  uint32_t _render_fb_tex;
  uint32_t _yuv_fbo;
  uint32_t _yuv_fb_tex;
  std::unique_ptr<uint8_t[]> _screen_data;

  struct {
    bool enabled;
    bool started;
    ovrSession session;
    ovrGraphicsLuid luid;
    ovrTextureSwapChain texture_chain;
    std::vector<uint32_t> fbo_ovr;
    ovrVector3f eye_view_offset[2];

    mutable ovrLayerEyeFov layer;
    mutable bool rendering_right;
  } _oculus;

  GLuint _new_program;
  GLuint _spiral_program;
  GLuint _yuv_program;
  GLuint _quad_buffer;

  std::unique_ptr<VisualApiImpl> _visual_api;
  std::unique_ptr<Visual> _visual;
  std::unique_ptr<Visual> _old_visual;
};

#endif
