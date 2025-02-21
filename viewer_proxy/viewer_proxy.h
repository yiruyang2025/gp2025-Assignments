#pragma once
#include <Eigen/Eigen>
#include <functional>

// Simple proxy around libigl's Viewer to cache the heavy compilation.
// We try to include all functions from libigl's Viewer
// (https://github.com/libigl/libigl/blob/main/include/igl/opengl/ViewerData.h)
// that you need, but we may have missed something. In that case you can add a
// the function here and in viewer_proxy.cpp following the examples that are
// already there. Editing the viewer_proxy.h/.cpp files will significantly slow
// down your compilation so we recommend to avoid editing it frequently.

class ViewerProxy {
public:
  // Proxy for igl::opengl::ViewerData, comments are copied from there.
  class Data {
  public:
    Data(void *data);
    /// Empty all fields
    void clear();
    /// Computes the normals of the mesh
    void compute_normals();
    /// Set the current mesh. Call this once at the beginning and whenever
    /// **both** the vertices and faces are changing.
    ///
    /// @param[in] V  #V by 3 list of mesh vertex positions
    /// @param[in] F  #F by 3/4 list of mesh faces (triangles/tets)
    void set_mesh(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);
    /// Set the normals of a mesh.
    ///
    /// @param[in] N #V|#F|3#F by 3 list of mesh normals
    ///
    /// \see compute_normals
    void set_normals(const Eigen::MatrixXd &N);
    /// Set the diffuse color of the mesh. The ambient color will be set to
    /// 0.1*C
    /// and the specular color will be set to 0.3+0.1*(C-0.3).
    ///
    /// @param[in] C  #V|#F|1 by 3 list of diffuse colors
    void set_colors(const Eigen::MatrixXd &colors);
    /// Add points given a list of point vertices.
    ///
    /// @param[in] P  #P by 3 list of vertex positions
    /// @param[in] C  #P|1 by 3 color(s)
    void add_points(const Eigen::MatrixXd &P, const Eigen::MatrixXd &C);
    /// Add edges given a list of edge start and end positions and colors
    ///
    /// @param[in] P1  #P by 3 list of edge start positions
    /// @param[in] P2  #P by 3 list of edge end positions
    /// @param[in] C  #P|1 by 3 color(s)
    void add_edges(const Eigen::MatrixXd &P, const Eigen::MatrixXd &E,
                   const Eigen::MatrixXd &C);
    /// Change whether drawing per-vertex or per-face; invalidating cache if
    /// necessary
    void set_face_based(bool face_based);
    /// Set per-vertex UV coordinates
    ///
    /// @param[in] UV  #V by 2 list of UV coordinates (indexed by F)
    void set_uv(const Eigen::MatrixXd &UV);
    /// Set whether this object is visible
    /// @param[in] value  true iff this object is visible
    /// @param[in] core_id  Index of the core to set (default is 0)
    void set_visible(bool visible, int core_id = 1);

    float &point_size;
    unsigned int &show_lines;
    unsigned int &show_faces;
    unsigned int &show_texture;
    void *_igl_viewer_data;
  };
  // Proxy for igl::opengl::ViewerCore.
  class Core {
  public:
    Core(void *igl_viewer_core);
    /// Adjust the view to see the entire model
    /// @param[in] V  #V by 3 list of vertex positions
    /// @param[in] F  #F by 3 list of triangle indices into V
    void align_camera_center(const Eigen::MatrixXd &V,
                             const Eigen::MatrixXi &F);
    /// Adjust the view to see the entire model
    /// @param[in] V  #V by 3 list of vertex positions
    /// @param[in] F  #F by 3 list of triangle indices into V
    void align_camera_center(const Eigen::MatrixXd &P);
    void disable_rotation();
    void *_igl_viewer_core;
    bool &orthographic;
    Eigen::Vector4f &viewport;
    Eigen::Matrix4f &proj;
    Eigen::Matrix4f &view;
    unsigned int &id;
  };

  // Proxy for igl::opengl::glfw::imgui::ImGuiMenu.
  class Menu {
  public:
    Menu();
    void draw_viewer_menu();

    std::function<void(void)> &callback_draw_viewer_menu;
  };

  static ViewerProxy &get_instance();

  void launch();
  // Return the current mesh, or the mesh corresponding to a given unique
  // identifier
  //
  // Inputs:
  //   mesh_id  unique identifier associated to the desired mesh (current mesh
  //   if -1)
  Data data(int mesh_id = -1);
  // Append a new "slot" for a mesh (i.e., create empty entries at the end of
  // the data_list and opengl_state_list.
  //
  // Inputs:
  //   visible  If true, the new mesh is set to be visible on all existing
  //   viewports
  // Returns the id of the last appended mesh
  Data append_mesh(bool visible = true);
  // Return the current viewport, or the viewport corresponding to a given
  // unique identifier
  //
  // Inputs:
  //   core_id  unique identifier corresponding to the desired viewport (current
  //   viewport if 0)
  Core core(unsigned core_id = 0);
  // Append a new "slot" for a viewport (i.e., copy properties of the current
  // viewport, only changing the viewport size/position)
  //
  // Inputs:
  //   viewport      Vector specifying the viewport origin and size in screen
  //   coordinates. append_empty  If true, existing meshes are hidden on the new
  //   viewport.
  Core append_core(const Eigen::Vector4f &viewport = Eigen::Vector4f::Zero());
  Menu &menu() { return _menu; }

  // Callbacks.
  std::function<bool(ViewerProxy &viewer, unsigned int key, int modifiers)>
      callback_key_down = [&](ViewerProxy &viewer, unsigned int key,
                              int modifiers) { return false; };
  std::function<bool(ViewerProxy &viewer, unsigned int key, int modifiers)>
      callback_mouse_down = [&](ViewerProxy &viewer, unsigned int key,
                              int modifiers) { return false; };
  std::function<bool(ViewerProxy &)> callback_init = [&](ViewerProxy &) {
    return false;
  };
  std::function<bool(ViewerProxy &, int, int)> callback_post_resize =
      [](ViewerProxy &, int, int) { return false; };
  int &current_mouse_x;
  int &current_mouse_y;

  /// Read mesh from an ascii file with automatic detection of file format
  /// among: mesh, msh obj, off, ply, stl, wrl.
  void load_mesh(const std::string &filename, Eigen::MatrixXd &V,
                 Eigen::MatrixXi &F);

protected:
  ViewerProxy();
  static ViewerProxy *_instance;
  Menu _menu;
};
