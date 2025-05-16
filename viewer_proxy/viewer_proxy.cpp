#include "viewer_proxy.h"
#include "igl/opengl/ViewerCore.h"
#include "igl/opengl/ViewerData.h"
#include <igl/read_triangle_mesh.h>

#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiHelpers.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>

#define CAST_DATA(data) ((igl::opengl::ViewerData *)data)
#define CAST_CORE(core) ((igl::opengl::ViewerCore *)core)

igl::opengl::glfw::Viewer viewer;
igl::opengl::glfw::imgui::ImGuiPlugin plugin;
igl::opengl::glfw::imgui::ImGuiMenu menu;
ViewerProxy *ViewerProxy::_instance = nullptr;

static void init_igl_viewer() {
  viewer.plugins.push_back(&plugin);
  plugin.widgets.push_back(&menu);
  viewer.core().set_rotation_type(
      igl::opengl::ViewerCore::ROTATION_TYPE_TRACKBALL);
}

ViewerProxy::ViewerProxy()
    : current_mouse_x(viewer.current_mouse_x),
      current_mouse_y(viewer.current_mouse_y) {}

ViewerProxy &ViewerProxy::get_instance() {
  if (_instance == nullptr) {
    _instance = new ViewerProxy();
    init_igl_viewer();

    viewer.callback_key_down = [&](igl::opengl::glfw::Viewer &viewer,
                                   unsigned char key, int modifiers) {
      return _instance->callback_key_down(*_instance, key, modifiers);
    };
    viewer.callback_mouse_down = [&](igl::opengl::glfw::Viewer &viewer,
                                     unsigned char key, int modifiers) {
      return _instance->callback_mouse_down(*_instance, key, modifiers);
    };
    viewer.callback_init = [&](igl::opengl::glfw::Viewer &viewer) {
      return _instance->callback_init(*_instance);
    };
    viewer.callback_post_resize = [&](igl::opengl::glfw::Viewer &viewer, int w,
                                      int h) {
      return _instance->callback_post_resize(*_instance, w, h);
    };
    viewer.callback_pre_draw = [&](igl::opengl::glfw::Viewer &viewer) {
      return _instance->callback_pre_draw(*_instance);
    };
  }
  return *_instance;
}

void ViewerProxy::launch() { viewer.launch(); }

void ViewerProxy::Data::clear() { CAST_DATA(_igl_viewer_data)->clear(); }
void ViewerProxy::Data::compute_normals() {
  CAST_DATA(_igl_viewer_data)->compute_normals();
}
void ViewerProxy::Data::set_mesh(const Eigen::MatrixXd &V,
                                 const Eigen::MatrixXi &F) {
  CAST_DATA(_igl_viewer_data)->set_mesh(V, F);
}
void ViewerProxy::Data::set_vertices(const Eigen::MatrixXd &V) {
  CAST_DATA(_igl_viewer_data)->set_vertices(V);
}
void ViewerProxy::Data::set_normals(const Eigen::MatrixXd &N) {
  CAST_DATA(_igl_viewer_data)->set_normals(N);
}
void ViewerProxy::Data::set_colors(const Eigen::MatrixXd &colors) {
  CAST_DATA(_igl_viewer_data)->set_colors(colors);
}

void ViewerProxy::Data::add_points(const Eigen::MatrixXd &P,
                                   const Eigen::MatrixXd &C) {
  CAST_DATA(_igl_viewer_data)->add_points(P, C);
}

void ViewerProxy::Data::add_edges(const Eigen::MatrixXd &P,
                                  const Eigen::MatrixXd &E,
                                  const Eigen::MatrixXd &C) {
  CAST_DATA(_igl_viewer_data)->add_edges(P, E, C);
}

void ViewerProxy::Data::set_face_based(bool face_based) {
  CAST_DATA(_igl_viewer_data)->set_face_based(face_based);
}

void ViewerProxy::Data::set_uv(const Eigen::MatrixXd &UV) {
  CAST_DATA(_igl_viewer_data)->set_uv(UV);
}

void ViewerProxy::Data::set_uv(const Eigen::MatrixXd &UV,
                               const Eigen::MatrixXi &FUV) {
  CAST_DATA(_igl_viewer_data)->set_uv(UV, FUV);
}

void ViewerProxy::Data::set_texture(
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &R,
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &G,
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &B) {
  CAST_DATA(_igl_viewer_data)->set_texture(R, G, B);
}

void ViewerProxy::Data::set_texture(
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &R,
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &G,
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &B,
    const Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> &A) {
  CAST_DATA(_igl_viewer_data)->set_texture(R, G, B, A);
}

void ViewerProxy::Data::set_visible(bool visible, int core_id) {
  CAST_DATA(_igl_viewer_data)->set_visible(visible, core_id);
}

ViewerProxy::Core::Core(void *igl_viewer_core)
    : _igl_viewer_core(igl_viewer_core),
      orthographic(CAST_CORE(igl_viewer_core)->orthographic),
      viewport(CAST_CORE(igl_viewer_core)->viewport),
      proj(CAST_CORE(igl_viewer_core)->proj),
      view(CAST_CORE(igl_viewer_core)->view),
      id(CAST_CORE(igl_viewer_core)->id),
      background_color(CAST_CORE(igl_viewer_core)->background_color),
      is_animating(CAST_CORE(igl_viewer_core)->is_animating),
      animation_max_fps(CAST_CORE(igl_viewer_core)->animation_max_fps) {}

void ViewerProxy::Core::align_camera_center(const Eigen::MatrixXd &V,
                                            const Eigen::MatrixXi &F) {
  ((igl::opengl::ViewerCore *)_igl_viewer_core)->align_camera_center(V, F);
}

void ViewerProxy::Core::align_camera_center(const Eigen::MatrixXd &P) {
  ((igl::opengl::ViewerCore *)_igl_viewer_core)->align_camera_center(P);
}

void ViewerProxy::Core::disable_rotation() {
  ((igl::opengl::ViewerCore *)_igl_viewer_core)
      ->set_rotation_type(igl::opengl::ViewerCore::ROTATION_TYPE_NO_ROTATION);
}

ViewerProxy::Data ViewerProxy::data(int mesh_id) {
  return Data(&viewer.data(mesh_id));
}

ViewerProxy::Data ViewerProxy::append_mesh(bool visible) {
  return data(viewer.append_mesh(visible));
}

ViewerProxy::Core ViewerProxy::core(unsigned core_id) {
  return Core(&viewer.core(core_id));
}
ViewerProxy::Core ViewerProxy::append_core(const Eigen::Vector4f &viewport) {
  return core(viewer.append_core(viewport));
}

ViewerProxy::Data::Data(void *data)
    : point_size(CAST_DATA(data)->point_size),
      show_lines(CAST_DATA(data)->show_lines),
      show_faces(CAST_DATA(data)->show_faces),
      show_texture(CAST_DATA(data)->show_texture), _igl_viewer_data(data) {}

ViewerProxy::Menu::Menu()
    : callback_draw_viewer_menu(::menu.callback_draw_viewer_menu) {}
void ViewerProxy::Menu::draw_viewer_menu() { ::menu.draw_viewer_menu(); }

void ViewerProxy::load_mesh(const std::string &filename, Eigen::MatrixXd &V,
                            Eigen::MatrixXi &F) {
  igl::read_triangle_mesh(filename, V, F);
}