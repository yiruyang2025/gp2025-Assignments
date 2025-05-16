#include "utils.h"
#include <Eigen/Eigen>
#include <fstream>
#include <igl/readOBJ.h>
#include <igl/stb/read_image.h>
#include <sys/stat.h>
#include <viewer_proxy.h>
using namespace Eigen;
using namespace std;

static const char *dirs_to_check[] = {"../data/",
                                      "data/",
                                      "assignment6/data/",
                                      "../assignment6/data/",
                                      "../../assignment6/data/",
                                      "../../../assignment6/data/"};

namespace utils {

std::string find_data_dir() {
  struct stat info;
  for (int i = 0; i < sizeof(dirs_to_check) / sizeof(dirs_to_check[0]); i++) {
    if (stat(dirs_to_check[i], &info) == 0 && info.st_mode & S_IFDIR) {
      return dirs_to_check[i];
    }
  }
  throw "Could not find data directory";
}

MeshData skeleton_mesh(const Eigen::MatrixXd &V, const Eigen::MatrixXd &V_tail,
                       double r, int res_x, int res_y, const Eigen::MatrixXd &C,
                       double shrink) {
  // Create a cylinder and a sphere.
  int pv = res_x * res_y;
  Eigen::MatrixXd V_cyl(pv, 3), V_sphere = V_cyl, VN_s = V_cyl, VN_c = V_cyl;
  for (int i = 0; i < res_x; i++) {
    double theta_2pi = 2 * PI * i / (res_x - 1);
    for (int j = 0; j < res_y; j++) {
      double phi = (double)j / (res_y - 1);
      double rr = r * std::max((1 - phi), shrink);

      V_cyl.row(i * res_y + j) << rr * cos(theta_2pi), rr * sin(theta_2pi), phi;
      VN_c.row(i * res_y + j) << cos(theta_2pi), sin(theta_2pi), 0;

      V_sphere.row(i * res_y + j) << r * cos(-theta_2pi) * sin(-PI * phi),
          r * sin(-theta_2pi) * sin(-PI * phi), r * cos(-PI * phi);
      VN_s.row(i * res_y + j) = V_sphere.row(i * res_y + j).normalized();
    }
  }
  // Grid triangle faces.
  Eigen::MatrixXi F_grid((res_x - 1) * (res_y - 1) * 2, 3);
  for (int i = 0; i < res_x - 1; i++) {
    for (int j = 0; j < res_y - 1; j++) {
      int v1 = i * res_y + j, v2 = (i + 1) * res_y + j,
          v3 = (i + 1) * res_y + (j + 1), v4 = i * res_y + (j + 1);
      F_grid.row(2 * (i * (res_y - 1) + j)) << v1, v2, v3;
      F_grid.row(2 * (i * (res_y - 1) + j) + 1) << v1, v3, v4;
    }
  }
  int nf = F_grid.rows(), ne = V.rows(), nv = 2 * ne;

  MeshData res;
  res.V.resize(pv * (nv + ne), 3);
  res.VN.resize(pv * (nv + ne), 3);
  res.C = Eigen::MatrixXd::Constant(pv * (nv + ne), C.cols(), 0.6);
  res.F.resize(nf * (nv + ne), 3);
  // Add spheres for the nodes.
  for (int i = 0; i < nv; i++) {
    if (i < ne) {
      res.V.middleRows(i * pv, pv) = V_sphere.rowwise() + V.row(i);
    } else {
      res.V.middleRows(i * pv, pv) =
          (shrink * V_sphere).rowwise() + V_tail.row(i - ne);
    }
    res.VN.middleRows(i * pv, pv) = VN_s;
    res.F.middleRows(i * nf, nf) = F_grid.array() + i * pv;
    if (C.rows() == ne || C.rows() == 1)
      res.C.middleRows(i * pv, pv) = C.row(i % C.rows()).replicate(pv, 1);
  }
  // Add cylinders for the edges.
  for (int i = 0; i < ne; i++) {
    Eigen::Matrix3d mat =
        Quaterniond::FromTwoVectors(V_tail.row(i) - V.row(i), Vector3d::UnitZ())
            .toRotationMatrix();
    // Update vertices.
    Eigen::MatrixXd V_tmp = V_cyl;
    V_tmp.col(2) *= (V_tail.row(i) - V.row(i)).norm();
    res.V.middleRows((nv + i) * pv, pv) = (V_tmp * mat).rowwise() + V.row(i);
    // Update normals.
    res.VN.middleRows((nv + i) * pv, pv) = (VN_c * mat);
    // Update faces.
    res.F.middleRows((nv + i) * nf, nf) = F_grid.array() + (nv + i) * pv;
    if (C.rows() == ne || C.rows() == 1)
      res.C.middleRows((nv + i) * pv, pv) =
          C.row(i % C.rows()).replicate(pv, 1);
  }
  return res;
}

void load_skeleton(const std::string &filename, Eigen::MatrixXd &bones,
                   Eigen::MatrixXi &bones_parents) {
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("Could not open file: " + filename);
  std::string line;
  std::vector<Eigen::Vector<double, 6>> bones_vec;
  std::vector<Eigen::Vector2i> parents;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string type;
    iss >> type;
    if (type == "b") {
      Eigen::Vector<double, 6> bone;
      iss >> bone[0] >> bone[1] >> bone[2] >> bone[3] >> bone[4] >> bone[5];
      bones_vec.push_back(bone);
    } else if (type == "p") {
      Eigen::Vector2i parent;
      iss >> parent[0] >> parent[1];
      parents.push_back(parent);
    }
  }
  file.close();
  bones = Eigen::Map<Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(
      bones_vec[0].data(), bones_vec.size(), 6);
  bones_parents = Eigen::Map<Eigen::Matrix<int, -1, -1, Eigen::RowMajor>>(
      parents[0].data(), parents.size(), 2);
}

static bool file_exists(const std::string &file_path) {
  struct stat info;
  if (stat(file_path.c_str(), &info) == 0 && info.st_mode & S_IFREG) {
    return true;
  }
  return false;
}

MeshData load_obj(const std::string &filename,
                  const std::string &texture_filename) {
  MeshData mesh_data;
  igl::readOBJ(filename, mesh_data.V, mesh_data.UV, mesh_data.VN, mesh_data.F,
               mesh_data.F_UV, mesh_data.FN);
  if (file_exists(texture_filename)) {
    igl::stb::read_image(texture_filename, mesh_data.texture_R,
                         mesh_data.texture_G, mesh_data.texture_B,
                         mesh_data.texture_A);
    mesh_data.C = Eigen::RowVector3d(1.0, 1.0, 1.0);
    mesh_data.has_texture = true;
  }
  return mesh_data;
}

} // namespace utils