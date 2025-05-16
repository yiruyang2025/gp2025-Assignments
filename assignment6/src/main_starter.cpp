#include "utils.h"
#include <Eigen/Eigen>
#include <chrono>
#include <igl/opengl/gl.h>
#include <imgui.h>
#include <viewer_proxy.h>
using namespace std;
using namespace std::chrono;
#define VIEWER ViewerProxy::get_instance()
#define CUT_TIME_MS                                                            \
  duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()

bool animating = false, backwards = false;
int anim_t = 0;
float r = 0.04, s = 0.5, fps = 0.0;
constexpr int N = 10;
// Edges of a path graph ([0, 1]; [1, 2]; ...; [N-2, N-1]).
const Eigen::MatrixXi E = Eigen::Matrix<int, N - 1, 2>::NullaryExpr(
    [](int i, int j) { return i + j; });
// Gradually changing color from black to red.
const auto C = Eigen::Matrix<double, N - 1, 3>::NullaryExpr(
    [](int i, int j) { return (double)i / (N - 2) * (j == 0); });
utils::MeshData mesh_data;

void gui() {
  ImGui::Text("Press space to start/stop animation, 'R' to reset");
  ImGui::Text("Up and down arrows to go forward/backward");
  ImGui::Separator();
  ImGui::Text("Animation time: %d ms", anim_t);
  ImGui::Text("FPS: %.2f", fps);
  ImGui::Checkbox("Animate", &animating);
  ImGui::SliderFloat("radius", &r, 0.01, 0.1);
  ImGui::SliderFloat("shrink", &s, 0.1, 1.0);
}

bool callback_key_down(ViewerProxy &viewer, unsigned char key, int modifiers) {
  if (key == ' ') {
    animating = !animating;
  } else if (key == 8 || key == 9) {
    backwards = (key == 8);
  } else if (key == 'R') {
    anim_t = 0;
  }
  return true;
}

bool callback_pre_draw(ViewerProxy &viewer) {
  static long last_time, fps_time, num_frames = -1;
  long time = CUT_TIME_MS, dt = (time - last_time);
  last_time = time;
  anim_t = max(0l, anim_t + (backwards ? -2 * dt : dt) * animating);
  if (++num_frames % 30 == 0) {
    fps = num_frames * 1000.0 / (time - fps_time);
    fps_time = time;
    num_frames = 0;
  }
  glEnable(GL_CULL_FACE); // For better transparency.

  Eigen::ArrayXd vt = Eigen::ArrayXd::LinSpaced(N, 0, 1) + anim_t * 0.0012 + .8;
  Eigen::ArrayXd theta = vt * 0.8,
                 phi = PI / 2 + (vt * 0.7).cos() * vt.sin() * PI / 3; // Walk.
  Eigen::MatrixXd V = Eigen::MatrixXd::Zero(N, 3);
  V << 2 * theta.cos() * phi.sin(), 2 * phi.cos(), 2 * theta.sin() * phi.sin();
  V.array().colwise() *= 1 + (0.18 * (vt * 5).sin()).max(0); // Jump.

  // Create a mesh from the vertices and edges.
  auto worm = utils::edges_mesh(V, E, r, 10, 10, C, s);
  VIEWER.data(0).set_mesh(worm.V, worm.F);
  VIEWER.data(0).set_normals(worm.VN); // Required for correct lighting.
  VIEWER.data(0).set_colors(worm.C);
  auto clamp = [](double x, double r) { return max(-r, min(r, x)); };
  double ang_x = clamp(asin(cos(phi(N - 1))) - PI / 17, PI / 6),
         ang_y = clamp(atan2(V(N - 1, 0), abs(V(N - 1, 2))), PI / 3);
  auto rot = Eigen::AngleAxisd(ang_x, Eigen::Vector3d::UnitX()) *
             Eigen::AngleAxisd(-ang_y, Eigen::Vector3d::UnitY());
  // *Only update vertices* (and potentially normals for correct lighting).
  VIEWER.data(1).set_vertices(mesh_data.V * rot.toRotationMatrix());
  VIEWER.data(1).set_normals(mesh_data.VN * rot.toRotationMatrix());
  // Alternatively (easier but slower): VIEWER.data(1).compute_normals();
  return false;
}

int main(int argc, char *argv[]) {
  VIEWER.callback_key_down = callback_key_down;
  VIEWER.callback_pre_draw = callback_pre_draw; // Animation stuff goes here.
  VIEWER.menu().callback_draw_viewer_menu = gui;
  VIEWER.data(0).show_lines = false;
  VIEWER.core().background_color.setOnes();

  mesh_data = utils::load_obj(utils::find_data_dir() + "sphere/rest.obj",
                              utils::find_data_dir() + "sphere/texture.png");
  ViewerProxy::Data sphere = VIEWER.append_mesh(); // VIEWER.data(1)
  sphere.set_mesh(mesh_data.V, mesh_data.F);
  if (mesh_data.has_texture) {
    sphere.set_uv(mesh_data.UV, mesh_data.F_UV);
    sphere.set_texture(mesh_data.texture_R, mesh_data.texture_G,
                       mesh_data.texture_B, mesh_data.texture_A);
    sphere.set_colors(mesh_data.C); // White to get the correct texture colors.
    sphere.show_texture = true;
  }
  sphere.show_lines = false;
  VIEWER.core().is_animating = true; // Call pre_draw continuously.
  // Not handled correctly on some Windows machines, potentially set higher if
  // the FPS is too low. And make sure you compile in RELEASE mode.
  VIEWER.core().animation_max_fps = 60;
  VIEWER.launch();
}
