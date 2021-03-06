#include "classdef.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <omp.h>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include "logistic.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <errno.h>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5File.hpp>









void controller::load_data_zscore(char *filename){


  using namespace HighFive;

  File file(filename, File::ReadOnly);

  auto snp_id_d = file.getDataSet("/z/SNP");
  auto loc_d = file.getDataSet("/z/locus");
  auto z_val_d = file.getDataSet("/z/z-val");
  std::vector<int> snp_v;
  std::vector<int> loc_v;
  std::vector<double> zval_v;

  snp_id_d.read(snp_v);
  loc_d.read(loc_v);
  z_val_d.read(zval_v);

 
  string line;
  istringstream ins;

  string curr_loc_id = "";

  vector<SNP> snp_vec;
  string loc_id;
  string snp_id;
  // double z_val;
  int index_count = 0;
  int loc_count = 0;
  size_t snpct = 0;

  const size_t num_rows = zval_v.size();
  for(size_t i=0; i<num_rows; i++){

    snp_id=std::to_string(snp_v[i]);
    loc_id=std::to_string(loc_v[i]);
    auto &z_val=zval_v[i];


      if(curr_loc_id != loc_id){
	if(curr_loc_id != ""){
	  
	  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
	    Locus loc(curr_loc_id, snp_vec);
	    int pos = loc_hash.size();
	    loc_hash[curr_loc_id] = pos;
	    locVec.push_back(loc);	  
	    loc_count++;
	  }else{
	    // already defined, concate the existing snpVec
	    int pos = loc_hash[curr_loc_id];
	    for(auto &tsnp : snp_vec){
              locVec[pos].snpVec.push_back(tsnp);
            }
	  }
	  
	  snp_vec.clear();
	}
	curr_loc_id = loc_id;
      }
      
      double log10_BF = compute_log10_BF(z_val);
      snp_hash[snp_id] =snpct;
      //printf("%15s   %7.3f\n", snp_id.c_str(), log10_BF);


      snp_vec.emplace_back(snpct, log10_BF, index_count);
            index_count++;
      snpct++;
  }


  if(loc_hash.find(curr_loc_id)== loc_hash.end()){
    Locus loc(curr_loc_id, snp_vec);
    int pos = loc_hash.size();
    loc_hash[curr_loc_id] = pos;
    locVec.push_back(loc);
    loc_count++;
  }else{
    // already defined, concate the existing snpVec                                               
    int pos = loc_hash[curr_loc_id];
    for(auto &tsnp : snp_vec){
      locVec[pos].snpVec.push_back(tsnp);
    }
  }
  
  

  p = index_count;

  fprintf(stderr, "Read in %d loci, %d locus-SNP pairs ... \n",loc_count, p);
  
  prior_vec = gsl_vector_calloc(p);
  
}













void controller::load_annotation(char* annot_file){

  using namespace HighFive;

  File file(annot_file, File::ReadOnly);


  //  auto cont_d = file.getDataSet("/annot_d/cont_m");
  auto cat_d = file.getDataSet("/annot_d/cat_m");

  auto feat_d_d = file.getDataSet("/annot/feature_names_d");
  //  auto feat_c_d = file.getDataSet("/annot/feature_names_c");
  auto data_d = cat_d.getSpace().getDimensions();
  const size_t p=data_d[0];
  kd = data_d[1];


  std::vector<std::string> fn;
  for(int i=0; i<kd; i++){
    dvar_name_vec.push_back(std::to_string(i));
  }
  //  feat_c_d.read(cvar_name_vec);
  //  feat_d_d.read(dvar_name_vec);
  kc =0;
  kd = dvar_name_vec.size();

  //  auto col_count = kc+kd;


  // read in data
  //  string snp;


  // memory allocation

// //  if(kc>0){
  //   Xc = gsl_matrix_calloc(p,kc);
  // }
  
 
  int ncol = kd;
  if(ncol>0){
    Xd = gsl_matrix_int_calloc(p,ncol);
    dlevel = gsl_vector_int_calloc(ncol);
  }

  cat_d.read(Xd->data);
  // for(int i=0; i<p; i++){
  //   gsl_matrix_int_set(Xd, i+1,0,tc[i]);
  // }

  //  cat_d.read(md);//memcpy( dest->data, src->data, cellsize * src_size1 * src_size2 * MULTIPLICITY )
  //  cont_c.read(Xc->data);


  
  // if(kc+kd>0){
  //   for (int i=0;i<locVec.size();i++){
  //     for(int j=0;j<locVec[i].snpVec.size();j++){
  // 	auto snp_id = locVec[i].snpVec[j].id;
  // 	int index = locVec[i].snpVec[j].index;
	
  // 	vector<double> avec((kd+kc),0.0);
  // 	if(annot_map.find(snp_id)!=annot_map.end())
  // 	  avec = annot_map[snp_id];

  // 	for(int k=0;k<avec.size();k++){
	  
  // 	  if(col2cat[k] == 1){
  // 	    gsl_matrix_set(Xc,index,col2cpos[k],avec[k]);
  // 	  }else{
  // 	    gsl_matrix_int_set(Xd, index, col2dpos[k],int(avec[k]));
  // 	  }
	
  // 	}
	
  //     }
  //   }
   
  for(int i=0;i<kd;i++){
      int nl = count_factor_level(i);

      gsl_vector_int_set(dlevel, i,nl);
    }
}



int controller::count_factor_level(int col){
 
  map<int, int> rcd;
  for(int i=0;i<p;i++){
    int val = gsl_matrix_int_get(Xd,i,col);
    rcd[val] = 1;
  }
  
  return rcd.size();
}



void controller::init_params(){
  
  prior_vec = gsl_vector_calloc(p);
  pip_vec = gsl_vector_calloc(p);
  for(int i=0;i<p;i++){
    gsl_vector_set(prior_vec, i, init_pi1);
  }
  

  for(int i=0;i<locVec.size();i++){
    locVec[i].pip_vec = pip_vec;
    locVec[i].prior_vec = prior_vec;
  }




  ncoef = 0;
  for(int i=0;i<kd;i++){
    ncoef += gsl_vector_int_get(dlevel,i)-1;
  }

  ncoef += 1;
  ncoef += kc;
  


  beta_vec = gsl_vector_calloc(ncoef);
  
}


void controller::run_EM(){  
  
  // start iteration
  fprintf(stderr,"Starting EM ... \n");
  int count = 1;
  double last_log10_lik = -9999999;
  init_params();


          
  


  int first_run = 1;
  // examine the stability for prob annotations
  
  while(1){
    
    
    double curr_log10_lik = 0;


    for(int i=0;i<locVec.size();i++){
   
      locVec[i].EM_update();
      curr_log10_lik += locVec[i].log10_lik;
               
    }

    if(first_run){
      
      if(single_fuzzy_annot == 1)
	single_probt_est();
      
      fprintf(stderr,"  Iter          loglik          Intercept    ");
	
      for(int i=0;i<dvar_name_vec.size();i++){
	string prefix = dvar_name_vec[i];
	int level = gsl_vector_int_get(dlevel,i);
	for(int j=1;j<level;j++){
	  ostringstream stream;
	  stream <<prefix<<"."<<j;
	    string label = stream.str();
	    fprintf(stderr, "%s\t", label.c_str());
	}
	
      }
      
      for(int i=0;i<cvar_name_vec.size();i++){
	fprintf(stderr, "%s\t", cvar_name_vec[i].c_str());
      }
      
    
      fprintf(stderr,"\n");


	
      first_run = 0;
    }



    if(ncoef==1){
      simple_regression();
    }
    // only categrical annotations
    else if(kc==0 && kd!=0){
      if(kd == 1 && !force_logistic){
	single_ct_regression();
      }else{
	logistic_cat_fit(beta_vec, Xd, dlevel,pip_vec, l1_lambda,l2_lambda);
      }
      
      logistic_cat_pred(beta_vec, Xd, dlevel,prior_vec);
    }
    // only continuous annotations
    else if(kc!=0 && kd==0){
      
      logistic_cont_fit(beta_vec, Xc, pip_vec,l1_lambda, l2_lambda);      
      logistic_cont_pred(beta_vec,Xc, prior_vec);
	
    }
    // both continuous and categorical annotations
    else if(kc!=0 && kd !=0){
      logistic_mixed_fit(beta_vec, Xd, dlevel, Xc, pip_vec, l1_lambda,l2_lambda);
      logistic_mixed_pred(beta_vec,Xd, dlevel, Xc, prior_vec);
    }

    //printf("%f   %f\n%f   %f\n",ct_00, ct_01, ct_10, ct_11);
    fprintf(stderr,"%4d        %10.3f        ",count++,curr_log10_lik/log10(exp(1)));
    

    for(int i=0;i<ncoef;i++){
      fprintf(stderr, "%9.3f  ",gsl_vector_get(beta_vec,i));
    }
    fprintf(stderr,"\n");
   
    // output 
    
    
    
    if(fabs(curr_log10_lik-last_log10_lik)<EM_thresh){
      final_log10_lik = curr_log10_lik;
      break;
    }

    last_log10_lik = curr_log10_lik;
  }
  
  
  double tp = 0;
  for(int i=0;i<p;i++){
    tp += gsl_vector_get(prior_vec,i);
  }
  if(print_avg)
    fprintf(stderr, "\nAvg. probability of association (%d unique variants): %9.3e\n", int(snp_hash.size()),tp/int(snp_hash.size()));
  
}



void controller::simple_regression(){
  
  double sum = 0;
  for(int i=0;i<p;i++){
    sum += gsl_vector_get(pip_vec,i);
  }
  double new_prior = sum/p;
  for(int i=0;i<p;i++){
    gsl_vector_set(prior_vec,i,new_prior);
  }

  gsl_vector_set(beta_vec,0, log(new_prior/(1-new_prior)));
}



void controller::single_probt_regression(){
  /*
  for(int i=0;i<p;i++){
  
    double qv = gsl_matrix_get(Xc,i,0);
    double pv = gsl_vector_get(pip_vec,i);
    printf("%7.3e  %7.3e\n",pv, qv);
  }
  exit(0);
  */
  double b0 = gsl_vector_get(beta_vec,0);
  double b1 = gsl_vector_get(beta_vec,1);
  
  //gsl_vector_set(beta_vec,1, b1);
  double p1 = exp(b0+b1)/(1+exp(b0+b1));
  double p0 = exp(b0)/(1+exp(b0));
  

  for(int i=0;i<p;i++){
    double qv = gsl_matrix_get(Xc,i,0);
    //double new_prior = qv*p1 + (1-qv)*p0;
    double new_prior = exp(b0+b1*qv)/(1+exp(b0+b1*qv));
    gsl_vector_set(prior_vec,i,new_prior);   
  }
  
}










void controller::single_probt_est(){
  
  if(!(kc==0&&kd==1) && !(kd==0&&kc==1))
    return;
  
  double c00 = 0;
  double c11 = 0;
  double c10 = 0;
  double c01 = 0;
  
  for(int i=0;i<p;i++){
    double qv;
    if(kc==1)
      qv = gsl_matrix_get(Xc,i,0);
    if(kd==1)
      qv = double(gsl_matrix_int_get(Xd,i,0));
    
    double pv = gsl_vector_get(pip_vec,i);
    c11 += pv*qv;
    c10 += pv*(1-qv);
    c01 += (1-pv)*qv;
    c00 += (1-pv)*(1-qv);
  }
  
  double sd = sqrt( 1/c11 + 1/c10 + 1/c01 + 1/c00);
  if(l2_lambda==0){
    l2_lambda = sd;
    fprintf(stderr, "Applying adaptive L2 penalty = %.3f\n\n", l2_lambda);
    //fprintf(stderr, "Set L2 penalty = %.3f\n", sd);
    force_logistic = 1;
  }
  //fprintf (stderr, "CT sd = %.3f  %.1f \n",sd, c11);
}



void controller::single_ct_regression(){
 
  map<int,double> sum_pip;
  map<int,double> sum;
  
  int levels = gsl_vector_int_get(dlevel,0);

  for(int i=0;i<levels;i++){
    sum_pip[i] = sum[i] = 0;
  }

  for(int i=0;i<p;i++){
    int cat = gsl_matrix_int_get(Xd,i,0);
    sum_pip[cat] += gsl_vector_get(pip_vec,i);
    sum[cat] += 1;
  }
  
  
  for(int i=0;i<p;i++){
    int cat = gsl_matrix_int_get(Xd,i,0);
    gsl_vector_set(prior_vec,i,sum_pip[cat]/sum[cat]);
  }
  

  double baseline=0;
  for(int i=0;i<levels;i++){
    double new_prior = sum_pip[i]/sum[i];
    gsl_vector_set(beta_vec,i, log(new_prior/(1-new_prior))-baseline);
    if(i==0){
      baseline = log(new_prior/(1-new_prior));
    }
 
  }
  
  
}




// option 1: find egene


void controller::find_eGene(double fdr_thresh){

  if(!finish_em){
    run_EM();
    finish_em = 1;
  }
  
  for(int i=0;i<locVec.size();i++){
    locVec[i].compute_fdr();
  }
  
  std::sort(locVec.begin(), locVec.end(),rank_by_fdr);
  
  double rej = 1.0;
  double cpr = 0.0;
  int rej_decision = 1;
  int rej_count = 0;
  for(int i=0;i<locVec.size();i++){
    cpr += locVec[i].fdr;
    if(cpr/rej > fdr_thresh){
      rej_decision = 0;
    }
    printf("%5d  %20s    %9.3e    %d\n",int(rej), locVec[i].id.c_str(), locVec[i].fdr,rej_decision);
    rej++;
    if(rej_decision == 1){
      rej_count++;
    }
  }
  
  fprintf(stderr,"\n\n    Total Loci: %d   Rejections: %d\n", int(locVec.size()), rej_count); 
  
}


// option 2: parameter estimation

void controller::estimate(){
  
  if(!finish_em){
    run_EM();
    finish_em = 1;
  }

  gsl_vector *saved_beta_vec = gsl_vector_calloc(ncoef);
  gsl_vector *saved_prior_vec = gsl_vector_calloc(p);
  gsl_vector_memcpy(saved_beta_vec, beta_vec);  
  gsl_vector_memcpy(saved_prior_vec, prior_vec);
  
  

  // CI for the intercept term
  double est = gsl_vector_get(beta_vec, 0);
  gsl_vector_set(beta_vec,0, 0.0);

  if(kc==0 && kd!=0){
    logistic_cat_pred(beta_vec, Xd, dlevel,prior_vec);
  }
  if(kc!=0 && kd==0){
    logistic_cont_pred(beta_vec,Xc, prior_vec);
  }
  if(kc!=0 && kd !=0){
    logistic_mixed_pred(beta_vec,Xd, dlevel, Xc, prior_vec);
  }
  
  if(kc==0 && kd ==0){
    
    for(int i=0;i<p;i++){
      gsl_vector_set(prior_vec,i,0.50);
    }
  }

  double null_log10_lik = 0;
  for(int k=0;k<locVec.size();k++){
    locVec[k].EM_update();
    null_log10_lik += locVec[k].log10_lik;
  }
  double diff = (final_log10_lik - null_log10_lik)/log10(exp(1));
  if(diff<1e-8){
    diff = 1e-8;
  }
  
  double sd = fabs(est)/sqrt(2*diff);
  printf("\n%25s  %9.3f     %9.3f  %9.3f\n", "Intercept", est, est-1.96*sd, est+1.96*sd);
  gsl_vector_memcpy(beta_vec, saved_beta_vec);
  gsl_vector_memcpy(prior_vec, saved_prior_vec);





  int index = 1; //start with 1, 0 always intercept
  
  
  
  for(int i=0;i<dvar_name_vec.size();i++){
    
    int level = gsl_vector_int_get(dlevel,i);
    string prefix = dvar_name_vec[i];
    for(int j=1;j<level;j++){
      ostringstream stream;
      if(prefix=="dtss"){
	int bin = dtss_rmap[j];
	stream<<"dtss."<<j<<"::["<<map_bin_2_dist(bin,dist_bin_size)<<"kb]";
      }else
	stream <<prefix<<"."<<j;
      string label = stream.str();
      double est = gsl_vector_get(beta_vec, index);
      gsl_vector_set(beta_vec,index, 0.0);

      if(kc==0 && kd!=0){
	logistic_cat_pred(beta_vec, Xd, dlevel,prior_vec);
      }
      if(kc!=0 && kd==0){
	logistic_cont_pred(beta_vec,Xc, prior_vec);
      }
      if(kc!=0 && kd !=0){
	logistic_mixed_pred(beta_vec,Xd, dlevel, Xc, prior_vec);
      }

      double null_log10_lik = 0;
     
      for(int k=0;k<locVec.size();k++){
	locVec[k].EM_update();
	null_log10_lik += locVec[k].log10_lik;
      }
  
      double diff = (final_log10_lik - null_log10_lik)/log10(exp(1));
      
      //printf("%f  %f  %f\n", null_log10_lik/log10(exp(1)), final_log10_lik/log10(exp(1)), diff);

      if(diff<0){
	double curr_log10_lik = final_log10_lik;
	est = fine_optimize_beta(index, est, null_log10_lik,  curr_log10_lik);
	diff = fabs(curr_log10_lik - null_log10_lik)/log10(exp(1));
      }
      
      if(diff<1e-8){
	diff = 1e-8;
      }
      double sd = fabs(est)/sqrt(2*diff);
      printf("%25s  %9.3f     %9.3f  %9.3f\n", label.c_str(), est, est-1.96*sd, est+1.96*sd);
      index++;
      // restore
      gsl_vector_memcpy(beta_vec, saved_beta_vec);  
      gsl_vector_memcpy(prior_vec, saved_prior_vec);
    }
  }
  
  
  for(int i=0;i<cvar_name_vec.size();i++){
    string label =  cvar_name_vec[i];
    double est = gsl_vector_get(beta_vec, index);
    gsl_vector_set(beta_vec,index, 0.0);
  
    
    if(kc==0 && kd!=0){
      logistic_cat_pred(beta_vec, Xd, dlevel,prior_vec);
    }
    if(kc!=0 && kd==0){
      logistic_cont_pred(beta_vec,Xc, prior_vec);
    }
    if(kc!=0 && kd !=0){
      logistic_mixed_pred(beta_vec,Xd, dlevel, Xc, prior_vec);
    }
    
    double null_log10_lik = 0;
    for(int k=0;k<locVec.size();k++){
      locVec[k].EM_update();
      null_log10_lik += locVec[k].log10_lik;
    }

    double diff = (final_log10_lik - null_log10_lik)/log10(exp(1));
    
    if(diff<0){
      double curr_log10_lik = final_log10_lik;
      est = fine_optimize_beta(index, est, null_log10_lik,  curr_log10_lik);
      diff = fabs(curr_log10_lik - null_log10_lik)/log10(exp(1));
    }
   
    if(diff<1e-8){
      diff = 1e-8;
    }
    double sd = fabs(est)/sqrt(2*diff);
    printf("%25s  %9.3f     %9.3f  %9.3f\n", label.c_str(), est, est-1.96*sd, est+1.96*sd);
    index++;
  }
  
  
  // restore all
  for(int k=0;k<locVec.size();k++){
    locVec[k].EM_update();
  }
  
  gsl_vector_free(saved_beta_vec);
  gsl_vector_free(saved_prior_vec);

}



void controller::dump_prior(char *prior_path){

  if(!finish_em){
    run_EM();
    finish_em = 1;
  }
 
  
  fprintf(stderr,"Output priors ...\n");
  if(mkdir(prior_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0){
    fprintf(stderr, "Error: cannot create directory %s\n", prior_path);
    exit(1);
  }
  
  string dir = string(prior_path);
  for( int i=0; i<locVec.size();i++){
    string file_name = dir + "/" + locVec[i].id+".prior";
    FILE *fd = fopen(file_name.c_str(), "w");
    for(int j=0;j<locVec[i].snpVec.size();j++){
      int index = locVec[i].snpVec[j].index;
      //      string name = locVec[i].snpVec[j].id;
      auto name = hash2snp[locVec[i].snpVec[j].id];
      fprintf(fd, "%s  %9.4e\n",name, gsl_vector_get(prior_vec, index));
    }
    fclose(fd);
  }
}


void controller::dump_pip(char *file){
  if(!finish_em){
    run_EM();
    finish_em = 1;
  }
  fprintf(stderr,"Output pip file: %s ...\n",file);
  FILE *fd = fopen(file, "w");
  for( int i=0; i<locVec.size();i++){
    string gname = locVec[i].id;
    for(int j=0;j<locVec[i].snpVec.size();j++){
      int index = locVec[i].snpVec[j].index;
      auto name = hash2snp[locVec[i].snpVec[j].id];
      fprintf(fd, "%s\t%s\t%9.4e\t%9.4e\n",name, gname.c_str(), gsl_vector_get(prior_vec, index), gsl_vector_get(pip_vec, index));
    }
  }
  fclose(fd);
}

						
    

double controller::fine_optimize_beta(int index, double est, double null_log10_lik, double &curr_log10_lik){
  
  double gr = (sqrt(5)-1)/2.0;
  
  /*
  double a = -fabs(est);
  double b = fabs(est);
  */

  double a = est;
  double b = 0;
  if(a > b){
    b = a;
    a = 0;
  }
  double c = b - gr*(b-a);
  double d = a + gr*(b-a);

  double thresh = 1e-3;
  if(fabs(c-d)<thresh){
    thresh = fabs(c-d)/2.0;
  }

  double fc;
  double fd;


  while((d-c)>thresh){
    
    fc = eval_likelihood(c, index);
    fd = eval_likelihood(d, index);

    
    //printf("%f %f %f %f   %f %f   %f\n",a, d
    if(fc > fd){
      /*
      if(fc > null_log10_lik){
	curr_log10_lik = fc;
	return c;
      }
      */
      b = d;
      d = c;
      c = b - gr*(b-a);
    }else{
      /*
      if(fd > null_log10_lik){
        curr_log10_lik = fd;
	return d;
      }
      */
      a = c;
      c = d;
      d = a + gr*(b-a);
    }
  }
  

  curr_log10_lik = fc;

  return (b+a)/2;

  
  
}



double controller::eval_likelihood(double x, int index){


  gsl_vector_set(beta_vec,index, x);

  if(kc==0 && kd!=0){
    logistic_cat_pred(beta_vec, Xd, dlevel,prior_vec);
  }
  if(kc!=0 && kd==0){
    logistic_cont_pred(beta_vec,Xc, prior_vec);
  }
  if(kc!=0 && kd !=0){
    logistic_mixed_pred(beta_vec,Xd, dlevel, Xc, prior_vec);
  }

  double log10_lik = 0;
  for(int k=0;k<locVec.size();k++){
    locVec[k].EM_update();
    log10_lik += locVec[k].log10_lik;
  }
  
  return log10_lik;
}





void Locus::EM_update(){

  
  // compute log10_lik
  vector<double> BF_vec;
  vector<double> p_vec;
  
  
  double locus_pi0 = 1;
  for(int i=0;i<snpVec.size();i++){
    double prior = gsl_vector_get(prior_vec, snpVec[i].index);
    BF_vec.push_back(snpVec[i].log10_BF);
    p_vec.push_back(prior/(1-prior));
    locus_pi0 *= (1-prior);
  }

  if(locus_pi0 < 1e-100){
    locus_pi0 = 1e-100;
  }
  
  for(int i=0;i<snpVec.size();i++){
    p_vec[i] *= locus_pi0;
  }
  
  BF_vec.push_back(0);
  p_vec.push_back(locus_pi0);

  /*
  for(int i=0;i<p_vec.size();i++){
    printf("%d  %7.3e\n",i, p_vec[i]);
  }
  */

  log10_lik = log10_weighted_sum(BF_vec, p_vec);
  
  //printf("%s  log10_lik = %f\n",id.c_str(), log10_lik);
  
  for(int i=0;i<snpVec.size();i++){
    
    double val = log10(p_vec[i]) + snpVec[i].log10_BF - log10_lik;
    double pip = pow(10, val);
    gsl_vector_set(pip_vec, snpVec[i].index, pip);
  }
  
 
}


void Locus::compute_fdr(){

  double locus_pi0 = 1;
 
  // compute log10_lik
  vector<double> BF_vec;
  vector<double> p_vec;
  for(int i=0;i<snpVec.size();i++){
    double prior = gsl_vector_get(prior_vec, snpVec[i].index);
    BF_vec.push_back(snpVec[i].log10_BF);
    p_vec.push_back(prior/(1-prior));
    locus_pi0 *= (1-prior);
  }
  
  for(int i=0;i<snpVec.size();i++){
    p_vec[i] *= locus_pi0;
  }


  BF_vec.push_back(0);
  p_vec.push_back(locus_pi0);
  
  log10_lik = log10_weighted_sum(BF_vec, p_vec);
  fdr =  pow(10,log10(locus_pi0)-log10_lik);

}





double compute_log10_BF(double beta, double se_beta){

  if(se_beta == 0)
    return 0;
  
  double phi[4] = {0.1,0.2,0.4,0.8};
  int size = 4;
  double z2 = pow((beta/se_beta), 2.0);
  double v2 = pow(se_beta, 2.0);
  vector<double> rstv;
  vector<double> wtv(size,1.0/double(size));
  for(int i=0;i<size;i++){
    //double w2 = pow(se_beta*phi[i],2.0);
    double w2 = pow(phi[i],2.0);
    double val = 0.5*log(v2/(v2+w2)) + 0.5*z2*(w2/(v2+w2));
    rstv.push_back(val/log(10));
  }

  return log10_weighted_sum(rstv,wtv);
}


double compute_log10_BF(double z_score){
  
  double kv[4] = {1,4,16,25};
  int size = 4;
  double z2 = pow(z_score, 2.0);
  vector<double> rstv;
  vector<double> wtv(size,1.0/double(size));
  for(int i=0;i<size;i++){
    //double w2 = pow(se_beta*phi[i],2.0);
    //double w2 = pow(phi[i],2.0);
    double val = 0.5*log(1/(1+kv[i])) + 0.5*z2*(kv[i]/(1+kv[i]));
    rstv.push_back(val/log(10));
  }

  return log10_weighted_sum(rstv,wtv);
}




double log10_weighted_sum(vector<double> &vec, vector<double> &wts){


  double max = vec[0];
  for(size_t i=0;i<vec.size();i++){
    if(vec[i]>max)
      max = vec[i];
  }
  double sum = 0;
  for(size_t i=0;i<vec.size();i++){
    sum += wts[i]*pow(10, (vec[i]-max));
  }

  return (max+log10(sum));
}

bool rank_by_fdr (const Locus & loc1 , const Locus & loc2){
  return (loc1.fdr < loc2.fdr);
}
 





// default binning scheme
int classify_dist_bin(int pos, int tss, double bin_size){

  if(bin_size>0){
    return int((pos-tss)/bin_size);
  }
  // else default scheme
  double dist = (pos-tss)/1000.0;
  int sign = 1;
  if(dist < 0){
    sign = -1;
  }
  dist = fabs(dist);
  int bin;
  
  if(dist<0.5){
    bin = 0;
  }else if(dist<1){
    bin = 1;
  }else if(dist<2.5){
    bin = 2;
  }else if(dist<5){
    bin = 3;
  }else if(dist<10){
    bin = 4;
  }else if(dist <25){
    bin = 5;
  }else if(dist <50){
    bin = 6;
  }else if(dist <100){
    bin = 7;
  }else if(dist <250){
    bin =8;
  }else if(dist < 500){
    bin = 9;
  }else{
    bin = 10;
  }

  return bin*sign;
  
}



double map_bin_2_dist(int bin, double bin_size){
 
  // return the mid point of the bin
  if(bin_size>0){
    if(bin == 0){
      return 0;
    }
    if(bin>0){
      return 0.5*(bin+1)*bin_size/1000;
    }
    if(bin<0){
      return 0.5*(bin-1)*bin_size/1000;
    }
  }
 

  // else default scheme
  int sign = 1;
  if(bin < 0){
    sign = -1;
  }

  if(bin == 0){
    return 0;
  }
  if(abs(bin)==1){
    return sign*0.75;
  }
  if(abs(bin)==2){
    return sign*1.75;
  }
  if(abs(bin)==3){
    return sign*3.75;
  }
  if(abs(bin)==4){
    return sign*7.5;
  }
  if(abs(bin)==5){
    return sign*17.5;
  }
  if(abs(bin)==6){
    return sign*37.5;
  }
  if(abs(bin)==7){
    return sign*75;
  }
  if(abs(bin)==8){
    return sign*175;
  }
  if(abs(bin)==9){
    return sign*375;
  }
  
  return sign*750;
  
  
}
