#ifndef TRANCE_THEME_H
#define TRANCE_THEME_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

namespace trance_pb {
  class Theme;
}

// In-memory image with load-on-request OpenGL texture which is ref-counted
// and automatically unloaded once no longer used.
struct Image {
  // In order to ensure textures are deleted from the rendering thread, we
  // use a separate set.
  static std::vector<unsigned int> textures_to_delete;
  static std::mutex textures_to_delete_mutex;
  static void delete_textures();

  struct texture_deleter {
    texture_deleter(unsigned int texture)
    : texture{texture} {}
    ~texture_deleter();
    unsigned int texture;
  };

  // Dummy values used to pass inject animations.
  enum {
    NONE,
    ANIMATION,
    ALTERNATE_ANIMATION
  } anim_type;

  Image(const std::string& path)
  : path{path}
  , width{0}
  , height{0}
  , texture{0}
  , anim_type{NONE} {}

  std::string path;
  unsigned int width;
  unsigned int height;
  std::shared_ptr<sf::Image> sf_image;
  mutable unsigned int texture;
  mutable std::shared_ptr<texture_deleter> deleter;
};

// Theme consists of images, animations, and associated text strings.
class Theme {
public:

  Theme(const trance_pb::Theme& proto);
  Theme(const Theme& theme);

  // Get a random loaded in-memory Image or text string.
  //
  // Note: get() is called from the main rendering thread and can upload
  // images from RAM to video memory on-demand. The other loading functions
  // are called from the async_update thread and load images from files
  // into RAM when requested.
  Image get() const;
  Image get_animation(std::size_t frame) const;
  const std::string& get_text() const;

  // Set the target number of images this set should keep in memory.
  // Once changed, the asynchronous image-loading thread will gradually
  // load/unload images until we're at the target.
  void set_target_load(std::size_t target_load);

  // Randomly swap out one in-memory image for another unloaded one.
  void perform_swap();
  // Perform at most one load or unload towards the target.
  void perform_load();
  // Perform all loads/unloads to reach the target.
  void perform_all_loads();

  // How many images are actually loaded.
  bool all_loaded() const;
  std::size_t loaded() const;

private:

  bool load_animation_gif_internal(std::vector<Image>& images,
                                   const std::string& path) const;
  bool load_animation_webm_internal(std::vector<Image>& images,
                                    const std::string& path) const;
  bool load_internal(Image* image, const std::string& path) const;

  void load_animation_internal();
  void unload_animation_internal();
  void load_internal();
  void unload_internal();

  Image get_internal(const std::vector<Image>& list, std::size_t index,
                     std::mutex& unlock) const;
  
  std::vector<std::string> _paths;
  std::vector<std::string> _texts;
  std::vector<Image> _images;
  std::size_t _target_load;
  mutable std::size_t _last_id;
  mutable std::size_t _last_text_id;
  mutable std::mutex _mutex;

  std::vector<std::string> _animation_paths;
  mutable std::size_t _animation_id;
  mutable std::mutex _animation_mutex;
  std::vector<Image> _animation_images;

};

// ThemeBank keeps two Themes active at all times with a number of images
// in memory each so that a variety of these images can be displayed with no
// load delay. It also asynchronously loads a third theme into memory so that
// the active themes can be swapped out.
class ThemeBank {
public:

  ThemeBank(const std::vector<trance_pb::Theme>& sets,
            unsigned int image_cache_size);

  // Get Images/text strings from either of the two active themes.
  Image get(bool alternate = false) const;
  const std::string& get_text(bool alternate = false) const;
  Image get_animation(std::size_t frame, bool alternate = false) const;

  // Call to upload a random image from the next theme which has been loaded
  // into RAM but not video memory.
  // This has to happen on the main rendering thread since OpenGL contexts
  // are single-threaded by default, but this function call can be timed to
  // mitigate the upload cost of switching active themes.
  void maybe_upload_next();

  // If the next theme has been fully loaded, swap it out for one of the two
  // active themes.
  bool change_themes();

  // Called from separate update thread to perform async loading/unloading.
  void async_update();

private:

  static const std::size_t switch_cooldown = 500;

  std::size_t _prev;
  std::size_t _a;
  std::size_t _b;
  std::size_t _next;

  std::vector<Theme> _themes;
  unsigned int _image_cache_size;
  unsigned int _updates;
  std::atomic<unsigned int> _cooldown;

};

#endif