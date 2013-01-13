#include <iostream>

#include "Cluster.h"
#include "utils.h"
#include "numerics.h"
#include "View.h"
#include "RandomNumberGenerator.h"

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

typedef boost::numeric::ublas::matrix<double> matrixD;
using namespace std;
typedef vector<Cluster*> vectorCp;
typedef set<Cluster*> setCp;
typedef map<int, Cluster*> mapICp;
typedef setCp::iterator setCp_it;
typedef mapICp::iterator mapICp_it;
typedef vector<int>::iterator vectorI_it;

void print_cluster_memberships(View& v) {
  cout << "Cluster memberships" << endl;
  setCp_it it = v.clusters.begin();
  for(; it!=v.clusters.end(); it++) {
    Cluster &cd = **it;
    cout << cd.get_row_indices() << endl;
  }
  cout << "num clusters: " << v.get_num_clusters() << endl;

}

void insert_and_print(View& v, map<int, vector<double> > data_map,
		      int cluster_idx, int row_idx) {
  vector<double> row = data_map[row_idx];
  Cluster& cluster = v.get_cluster(cluster_idx);
  v.insert_row(row, cluster, row_idx);
  cout << "v.insert(" << row << ", " << cluster_idx << ", "	\
	    << row_idx << ")" << endl;
  cout << "v.get_score(): " << v.get_score() << endl;
}

void remove_all_data(View &v, map<int, vector<double> > data_map) {
  vector<int> rows_in_view;
  for(mapICp_it it=v.cluster_lookup.begin(); it!=v.cluster_lookup.end(); it++) {
    rows_in_view.push_back(it->first);
  }
  for(vectorI_it it=rows_in_view.begin(); it!=rows_in_view.end(); it++) {
    int idx_to_remove = *it;
    vector<double> row = data_map[idx_to_remove];
    v.remove_row(row, idx_to_remove);
  }
  cout << "removed all data" << endl;
  v.print();
  //
  for(setCp_it it=v.clusters.begin(); it!=v.clusters.end(); it++) {
    v.remove_if_empty(**it);
  }
  assert(v.get_num_vectors()==0);
  assert(v.get_num_clusters()==0);
  cout << "removed empty clusters" << endl; 
  v.print();
}

int main(int argc, char** argv) {
  cout << endl << "Hello World!" << endl;

  // load some data
  matrixD data;
  LoadData("Synthetic_data.csv", data);
  int num_cols = data.size2();
  double init_crp_alpha = 3;
  //
  map<int, vector<double> > data_map;
  cout << "populating data_map" << endl;
  for(int idx=0; idx<6; idx++) {
    vector<double> row = extract_row(data, idx);
    cout << "row_idx: " << idx << "; data: " << row << endl;
    data_map[idx] = row;
  }
  //
  map<int, int> where_to_push;
  where_to_push[0] = 0;
  where_to_push[1] = 1;
  where_to_push[2] = 0;
  where_to_push[3] = 1;
  where_to_push[4] = 0;
  where_to_push[5] = 1;
  //
  set<int> cluster_idx_set;
  for(map<int,int>::iterator it=where_to_push.begin(); it!=where_to_push.end(); it++) {
    int cluster_idx = it->second;
    cluster_idx_set.insert(cluster_idx);
  }

  // create the objects to test
  // View v = View(num_cols, init_crp_alpha);
  vector<int> global_column_indices;
  for(int col_idx=0; col_idx<data.size2(); col_idx++) {
    global_column_indices.push_back(col_idx);
  }
  View v = View(data, global_column_indices, 11);
  v.print();
  v.assert_state_consistency();
  //
  vectorCp cd_v;
  for(set<int>::iterator it=cluster_idx_set.begin(); it!=cluster_idx_set.end(); it++) {
    int cluster_idx = *it;
    cout << "inserting cluster idx: " << cluster_idx << endl;
    Cluster *p_cd = new Cluster(num_cols);
    cd_v.push_back(p_cd);
  }

  // print the initial view
  cout << "empty view print" << endl;
  v.print();
  cout << "empty view print" << endl;
  cout << endl;
  
  // populate the objects to test
  cout << endl << "populating objects" << endl;
  cout << "=================================" << endl;
  for(map<int,int>::iterator it=where_to_push.begin(); it!=where_to_push.end(); it++) {
    v.assert_state_consistency();
    int row_idx = it->first;
    int cluster_idx = it->second;
    cout << "INSERTING ROW: " << row_idx << endl;
    insert_and_print(v, data_map, cluster_idx, row_idx);
    Cluster *p_cd = cd_v[cluster_idx];
    double cluster_score_delta = (*p_cd).insert_row(data_map[row_idx], row_idx);
    cout << "cluster_score_delta: " << cluster_score_delta << endl;
    cout << "DONE INSERTING ROW: " << row_idx << endl;
  }
  cout << endl << "view after population" << endl;
  v.print();
  cout << "view after population" << endl;
  cout << "=================================" << endl;
  cout << endl;

  // print the clusters post population
  cout << endl << "separately created clusters after population" << endl;
  for(vectorCp::iterator it=cd_v.begin(); it!=cd_v.end(); it++) {
    cout << **it << endl;
  }
  cout << endl;

  // independently verify view score as sum of data and crp scores
  vector<double> cluster_scores;
  setCp_it it = v.clusters.begin();
  double sum_scores = 0;
  for(; it!=v.clusters.end(); it++) {
    double cluster_score = (*it)->calc_sum_marginal_logps();
    cluster_scores.push_back(cluster_score);
    sum_scores += cluster_score;
  }
  vector<int> cluster_counts = v.get_cluster_counts();
  double crp_score = numerics::calc_crp_alpha_conditional(cluster_counts,
							  v.get_crp_alpha(),
							  -1, true);
  double crp_plus_data_score = crp_score + sum_scores;
  cout << "vector of cluster scores: " << cluster_scores << endl;
  cout << "sum cluster scores: " << sum_scores << endl;
  cout << "crp score: " << crp_score << endl;
  cout << "sum cluster scores and crp score: " << crp_plus_data_score << endl;
  cout << "view score: " << v.get_score() << endl;
  assert(is_almost(v.get_score(), crp_plus_data_score, 1E-10));

  // test crp alpha hyper inference
  vector<double> test_alphas;
  test_alphas.push_back(.3);
  test_alphas.push_back(3.);
  test_alphas.push_back(30.);
  cluster_counts = v.get_cluster_counts();  
  vector<double> test_alpha_scores;
  for(vector<double>::iterator it=test_alphas.begin(); it!=test_alphas.end(); it++) {
    double test_alpha_score = numerics::calc_crp_alpha_conditional(cluster_counts, *it, -1, true);
    test_alpha_scores.push_back(test_alpha_score);
    v.assert_state_consistency();
  }
  cout << "test_alphas: " << test_alphas << endl;
  cout << "test_alpha_scores: " << test_alpha_scores << endl;
  double new_alpha = test_alphas[0];
  double crp_score_delta = v.set_alpha(new_alpha);
  cout << "new_alpha: " << new_alpha << ", new_alpha score: " << v.get_crp_score() << ", crp_score_delta: " << crp_score_delta << endl;
  new_alpha = test_alphas[1];
  crp_score_delta = v.set_alpha(new_alpha);
  cout << "new_alpha: " << new_alpha << ", new_alpha score: " << v.get_crp_score() << ", crp_score_delta: " << crp_score_delta << endl;

  // test continuous data hyper conditionals
  vector<double> hyper_grid;
  vector<double> hyper_logps;
  int N_GRID = 11;
  vector<string> hyper_strings;
  hyper_strings.push_back("r");  hyper_strings.push_back("nu");  hyper_strings.push_back("s");  hyper_strings.push_back("mu");
  map<string, double> default_hyper_values;
  default_hyper_values["r"] = 1.0; default_hyper_values["nu"] = 2.0; default_hyper_values["s"] = 2.0; default_hyper_values["mu"] = 0.0;
  for(vector<string>::iterator it = hyper_strings.begin(); it!=hyper_strings.end(); it++) {
    vector<double> curr_r_conditionals;
    string hyper_string = *it;
    double default_value = default_hyper_values[hyper_string];
    cout << endl;
    cout << hyper_string << " hyper conditionals" << endl;
    cout << "num_cols: " << num_cols << endl;
    for(int col_idx=0; col_idx<num_cols; col_idx++) {
      hyper_grid = v.get_hyper_grid(col_idx, hyper_string);
      cout << hyper_string << " grid: " << hyper_grid << endl;
      hyper_logps = v.calc_hyper_conditionals(col_idx, hyper_string, hyper_grid);
      cout << "conditionals: " << hyper_logps << endl;
      double curr_r_conditional = hyper_logps[(int)(N_GRID-1)/2];
      curr_r_conditionals.push_back(curr_r_conditional);
      cout << "curr conditional: " << curr_r_conditional << endl;
    }
    double sum_curr_conditionals = std::accumulate(curr_r_conditionals.begin(),curr_r_conditionals.end(),0.);
    cout << "sum curr conditionals: " << sum_curr_conditionals << endl;
  }

  // test continuous data hyper inference
  // verify setting results in predicted delta
  cout << endl;
  cout << endl;
  string hyper_string = "r";
  double default_value = default_hyper_values[hyper_string];
  //
  hyper_grid = v.get_hyper_grid(0, "r");
  int col_idx = 0;
  vector<double> unorm_logps = v.calc_hyper_conditionals(col_idx, hyper_string, hyper_grid);
  double curr_conditional = unorm_logps[(int)(N_GRID-1)/2];
  double curr_data_score = v.get_data_score();
  //
  cout << "hyper_grid: " << hyper_grid << endl;
  cout << "unorm_logps: " << unorm_logps << endl;
  vector<double> score_deltas = unorm_logps;
  for(vector<double>::iterator it=score_deltas.begin(); it!=score_deltas.end(); it++) {
    *it -= curr_conditional;
  }
  cout << "score_deltas: " << score_deltas << endl;
  double data_score_0 = v.get_data_score();
  for(int grid_idx=0; grid_idx<hyper_grid.size(); grid_idx++) {
    double new_hyper_value = hyper_grid[grid_idx];
    v.set_hyper(col_idx, hyper_string, new_hyper_value);
    double new_data_score = v.get_data_score();
    double data_score_delta = new_data_score - data_score_0;
    cout << "hyper_value: " << new_hyper_value << ", data_score: " << new_data_score << ", data_score_delta: " << data_score_delta << endl;
  }

  
  // print state info before transitioning
  print_cluster_memberships(v);
  int num_vectors = v.get_num_vectors();
  cout << "num_vectors: " << v.get_num_vectors() << endl;
  //
  cout << "====================" << endl;
  cout << "Sampling" << endl;

  // test transition
  RandomNumberGenerator rng = RandomNumberGenerator();
  for(int iter=0; iter<21; iter++) {
    v.assert_state_consistency();
    v.transition_zs(data_map);
    v.transition_crp_alpha();
    for(int col_idx=0; col_idx<num_cols; col_idx++) {
      std::random_shuffle(hyper_strings.begin(), hyper_strings.end());
      for(vector<string>::iterator it=hyper_strings.begin(); it!=hyper_strings.end(); it++) {
	string hyper_string = *it;
	v.transition_hyper_i(col_idx,hyper_string);
      }
    }
    // if(iter % 10 == 0) {
    if(iter % 1 == 0) {
      print_cluster_memberships(v);
      for(int col_idx=0; col_idx<num_cols; col_idx++) {
	cout << "Hypers(col_idx=" << col_idx <<"): " << v.get_hypers(col_idx) << endl;
      }
      cout << "score: " << v.get_score() << endl;
      cout << "Done iter: " << iter << endl;
      cout << endl;
    }
  }
  print_cluster_memberships(v);
  cout << "Done transition_zs" << endl;
  cout << endl;

  // empty object and verify empty
  remove_all_data(v, data_map);
  v.print();

  for(vectorCp::iterator it = cd_v.begin(); it!=cd_v.end(); it++) {
    delete (*it);
  }

  cout << endl << "Goodbye World!" << endl;
}
