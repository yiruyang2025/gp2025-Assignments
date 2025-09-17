#include <iostream>
#include <sys/stat.h>

// libigl headers
#include <igl/readOFF.h>
#include <igl/vertex_triangle_adjacency.h>
#include <igl/adjacency_list.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>
#include <igl/per_corner_normals.h>
#include <igl/facet_components.h>
#include <igl/jet.h>
#include <igl/barycenter.h>
#include <igl/edge_topology.h>
#include <cmath>

#include <imgui.h>
#include <viewer_proxy.h>

// Vertex array, #V x3
Eigen::MatrixXd V;
// Face array, #F x3
Eigen::MatrixXi F;
// Per-face normal array, #F x3
Eigen::MatrixXd FN;
// Per-vertex normal array, #V x3
Eigen::MatrixXd VN;
// Per-corner normal array, (3#F) x3
Eigen::MatrixXd CN;
// Vectors of indices for adjacency relations
std::vector<std::vector<int>> VF, VFi, VV;
// Integer vector of component IDs per face, #F x1
Eigen::VectorXi cid;
// Per-face color array, #F x3
Eigen::MatrixXd component_colors_per_face;

// --- sqrt(3) subdivision placeholder ---
void subdivide_sqrt3(const Eigen::MatrixXd &Vin, const Eigen::MatrixXi &Fin,
                     Eigen::MatrixXd &Vout, Eigen::MatrixXi &Fout) {
  // Implement one iteration of Kobbelt's sqrt(3) subdivision for triangle meshes.
  // Reference rules:
  // 1) For every face f add one new vertex at the face barycenter (call it b_f).
  // 2) Relocate each original vertex v to a new position p using
  //      p = (1 - a_n) * v + (a_n / n) * sum(neighbors of v)
  //    where n is the valence of v and a_n = (4 - 2*cos(2*pi/n)) / 9.
  //    Boundary vertices stay at their original positions.
  // 3) Replace the original connectivity by flipping all original edges (edges
  //    between old vertices). The edges that connect barycenters to original
  //    vertices remain. For boundary edges (with only one adjacent face) we do
  //    not flip; keep the triangle of the split face.

  const int numOriginalVertices = static_cast<int>(Vin.rows());
  const int numFaces = static_cast<int>(Fin.rows());

  // --- Step 1: Compute one barycenter per face and append to the vertex list.
  Eigen::MatrixXd BC; // #F x 3
  igl::barycenter(Vin, Fin, BC);

  // --- Step 2: Compute vertex-vertex adjacency to relocate original vertices.
  std::vector<std::vector<int>> vertexToVertex;
  igl::adjacency_list(Fin, vertexToVertex);

  // Boundary detection using edge topology (an edge with one adjacent face is boundary).
  Eigen::MatrixXi EV; // #E x 2, vertex indices of each edge
  Eigen::MatrixXi FE; // #F x 3, edge index for each face corner
  Eigen::MatrixXi EF; // #E x 2, adjacent faces for each edge (-1 for boundary)
  igl::edge_topology(Vin, Fin, EV, FE, EF);

  std::vector<char> isBoundaryVertex(numOriginalVertices, 0);
  for (int e = 0; e < EF.rows(); ++e) {
    if (EF(e, 0) == -1 || EF(e, 1) == -1) {
      // Mark both endpoints of a boundary edge as boundary vertices
      isBoundaryVertex[EV(e, 0)] = 1;
      isBoundaryVertex[EV(e, 1)] = 1;
    }
  }

  // Relocate original vertices
  Eigen::MatrixXd Vrelocated(numOriginalVertices, 3);
  const double PI = 3.14159265358979323846;
  for (int v = 0; v < numOriginalVertices; ++v) {
    if (isBoundaryVertex[v]) {
      // Boundary vertices stay at original position
      Vrelocated.row(v) = Vin.row(v);
      continue;
    }
    const std::vector<int> &nbrs = vertexToVertex[v];
    const int n = static_cast<int>(nbrs.size());
    if (n == 0) {
      Vrelocated.row(v) = Vin.row(v);
      continue;
    }
    // a_n according to the formula in the assignment handout
    const double a_n = (4.0 - 2.0 * std::cos(2.0 * PI / static_cast<double>(n))) / 9.0;
    Eigen::RowVector3d sumNbr(0.0, 0.0, 0.0);
    for (int u : nbrs) sumNbr += Vin.row(u);
    Vrelocated.row(v) = (1.0 - a_n) * Vin.row(v) + (a_n / static_cast<double>(n)) * sumNbr;
  }

  // Concatenate relocated originals with barycenters to build Vout
  Vout.resize(numOriginalVertices + numFaces, 3);
  Vout.topRows(numOriginalVertices) = Vrelocated;
  Vout.bottomRows(numFaces) = BC;

  // Helper to map face index -> added barycenter vertex index in Vout
  auto baryIdx = [&](int f) { return numOriginalVertices + f; };

  // --- Step 3: Build new faces by flipping original edges.
  // For interior edges shared by faces f0 and f1, create two triangles:
  //   (b_f0, v0, b_f1) and (b_f0, v1, b_f1), where edge is (v0, v1).
  // For boundary edges (only one adjacent face fb), keep one triangle:
  //   (b_fb, v0, v1).
  int numInteriorEdges = 0, numBoundaryEdges = 0;
  for (int e = 0; e < EF.rows(); ++e) {
    if (EF(e, 0) != -1 && EF(e, 1) != -1) numInteriorEdges++;
    else numBoundaryEdges++;
  }

  const int numNewFaces = 2 * numInteriorEdges + numBoundaryEdges;
  Fout.resize(numNewFaces, 3);

  int fptr = 0;
  for (int e = 0; e < EF.rows(); ++e) {
    const int v0 = EV(e, 0);
    const int v1 = EV(e, 1);
    const int f0 = EF(e, 0);
    const int f1 = EF(e, 1);

    if (f0 != -1 && f1 != -1) {
      // Interior edge: create two triangles (b_f0, v0, b_f1) and (b_f0, v1, b_f1)
      Fout.row(fptr++) << baryIdx(f0), v0, baryIdx(f1);
      Fout.row(fptr++) << baryIdx(f0), v1, baryIdx(f1);
    } else {
      // Boundary edge: keep the triangle from the single adjacent face
      const int fb = (f0 != -1) ? f0 : f1;
      Fout.row(fptr++) << baryIdx(fb), v0, v1;
    }
  }
}

// --- Key callback ---
bool callback_key_down(ViewerProxy &viewer, unsigned char key, int modifiers) {
  if (key == '1') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 1 pressed: Vertex-to-Face adjacency" << std::endl;

    igl::vertex_triangle_adjacency(V, F, VF, VFi);
    for (int v = 0; v < VF.size(); v++) {
      std::cout << "Vertex " << v << " -> faces: ";
      for (int f : VF[v]) std::cout << f << " ";
      std::cout << std::endl;
    }
  }

  if (key == '2') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 2 pressed: Vertex-to-Vertex adjacency" << std::endl;

    igl::adjacency_list(F, VV);
    for (int v = 0; v < VV.size(); v++) {
      std::cout << "Vertex " << v << " -> neighbors: ";
      for (int u : VV[v]) std::cout << u << " ";
      std::cout << std::endl;
    }
  }

  if (key == '3') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 3 pressed: Per-face normals" << std::endl;

    igl::per_face_normals(V, F, FN);
    viewer.data().set_normals(FN);
  }

  if (key == '4') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 4 pressed: Per-vertex normals" << std::endl;

    igl::per_vertex_normals(V, F, VN);
    viewer.data().set_normals(VN);
  }

  if (key == '5') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 5 pressed: Per-corner normals" << std::endl;

    igl::per_corner_normals(V, F, 20, CN); // 20° threshold
    viewer.data().set_normals(CN);
  }

  if (key == '6') {
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
    std::cout << "Key 6 pressed: Connected components" << std::endl;

    igl::facet_components(F, cid);
    igl::jet(cid.cast<double>(), true, component_colors_per_face);
    viewer.data().set_colors(component_colors_per_face);
  }

  if (key == '7') {
    std::cout << "Key 7 pressed: sqrt(3) subdivision (placeholder)" << std::endl;
    Eigen::MatrixXd Vout;
    Eigen::MatrixXi Fout;

    subdivide_sqrt3(V, F, Vout, Fout);
    V = Vout;
    F = Fout;
    viewer.data().clear();
    viewer.data().set_mesh(V, F);
  }

  return true;
}

// --- Load mesh ---
bool load_mesh(ViewerProxy &viewer, std::string filename,
               Eigen::MatrixXd &V, Eigen::MatrixXi &F) {
  igl::readOFF(filename, V, F);
  viewer.data().clear();
  viewer.data().set_mesh(V, F);
  viewer.data().compute_normals();
  viewer.core().align_camera_center(V, F);
  return true;
}

// --- Find data directory ---
const char *dirs_to_check[] = {"../data/",
                               "data/",
                               "assignment1/data/",
                               "../assignment1/data/",
                               "../../assignment1/data/",
                               "../../../assignment1/data/"};

std::string find_data_dir() {
  static struct stat info;
  for (int i = 0; i < sizeof(dirs_to_check) / sizeof(dirs_to_check[0]); i++) {
    if (stat(dirs_to_check[i], &info) == 0 && info.st_mode & S_IFDIR) {
      return dirs_to_check[i];
    }
  }
  throw "Could not find data directory";
}

// --- Main ---
int main(int argc, char *argv[]) {
  ViewerProxy &viewer = ViewerProxy::get_instance();
  viewer.callback_key_down = callback_key_down;

  std::string filename;
  if (argc == 2) {
    filename = std::string(argv[1]); // Mesh from command line
  } else {
    filename = find_data_dir() + "/bunny.off"; // Fallback
  }

  load_mesh(viewer, filename, V, F);
  callback_key_down(viewer, '1', 0);

  // GUI menu setup
  viewer.menu().callback_draw_viewer_menu = [&]() {
    viewer.menu().draw_viewer_menu();
    ImGui::Separator();
    if (ImGui::Button("I'm a button")) {
      std::cout << "and I was just clicked" << std::endl;
    }
  };

  viewer.launch();
}

