/**
 * @author Hua Ouyang
 *
 * @file opt_cd.h
 *
 * This head file contains functions for performing Dual Coordinate Descent based optimization for linear L1- and L2- SVMs
 *
 * The algorithms in the following paper is implemented:
 *
 * 1. Dual Coordinate Descent for L1- and L2-SVM
 * @ARTICLE{Hsieh_DCD,
 * author = "Cho-Jui Hsieh, Kai-Wei Chang, Chih-Jen Lin",
 * title = "{A Dual Coordinate Descent Method for Large Scale Linear SVM}",
 * booktitle = ICML,
 * year = 2008,
 * }
 *
 * @see svm.h
 */

#ifndef U_SVM_OPT_CD_H
#define U_SVM_OPT_CD_H

#include "fastlib/fastlib.h"

// threshold that determines whether an alpha is a SV or not
const double CD_ALPHA_ZERO = 1.0e-7;


template<typename TKernel>
class CD {
  FORBID_ACCIDENTAL_COPIES(CD);

 public:
  typedef TKernel Kernel;

 private:
  int learner_typeid_;
  int regularization_; // do L2-SVM or L1-SVM, default: L1

  Kernel kernel_;
  const Dataset *dataset_;
  index_t n_data_; /* number of data samples */
  index_t n_features_; /* # of features == # of row - 1, exclude the last row (for labels) */
  index_t n_features_bias_; /* # of features + 1 , for the bias term */
  Matrix datamatrix_; /* alias for the data matrix */

  Vector alpha_; /* the Lagrangian multipliers */

  Vector coef_; /* alpha*y, to be optimized */
  index_t n_alpha_; /* number of lagrangian multipliers in the dual */
  
  index_t i_cache_, j_cache_; /* indices for the most recently cached kernel value */
  double cached_kernel_value_; /* cache */

  ArrayList<int> y_; /* list that stores "labels" */

  Vector w_; /* the slope of the decision hyperplane y=w^T x+b */
  double bias_;

  // parameters
  double C_; // for SVM_C
  double Cp_; // C for positive samples
  double Cn_; // C for negative samples
  double epsilon_; // for SVM_R

  double lambda_; // regularization parameter. lambda = 1/(C*n_data)
  index_t n_iter_; // number of iterations
  index_t n_epochs_; // number of epochs
  double accuracy_; // accuracy for stopping creterion
  double t_;

 public:
  CD() {}
  ~CD() {}

  /**
   * Initialization for parameters
   */
  void InitPara(int learner_typeid, ArrayList<double> &param_) {
    // init parameters
    if (learner_typeid == 0) { // SVM_C
      Cp_ = param_[0];
      Cn_ = param_[1];
      DEBUG_ASSERT(Cp_ != 0);
      DEBUG_ASSERT(Cn_ != 0);
      regularization_ = (int) param_[2];
      n_epochs_ = (index_t)param_[3];
      n_iter_ = (index_t)param_[4];
      accuracy_ = param_[5];
    }
    else if (learner_typeid == 1) { // SVM_R
    }
  }

  void Train(int learner_typeid, const Dataset* dataset_in);

  Kernel& kernel() {
    return kernel_;
  }

  double Bias() const {
    return bias_;
  }

  Vector* GetW() {
    return &w_;
  }


 private:
  /**
   * Loss functions
   */
  double LossFunction_(int learner_typeid, double yy_hat) {
    if (learner_typeid_ == 0) { // SVM_C
      return HingeLoss_(yy_hat);
    }
    else if (learner_typeid_ == 1) { // SVM_R
      return 0.0; // TODO
    }
    else
      return HingeLoss_(yy_hat);
  }

  /**
   * Gradient of loss functions
   */
  double LossFunctionGradient_(int learner_typeid, double yy_hat) {
    if (learner_typeid_ == 0) { // SVM_C
      return HingeLossGradient_(yy_hat);
    }
    else if (learner_typeid_ == 1) { // SVM_R
      return 0.0; // TODO
    }
    else {
      if (yy_hat < 1.0)
	return 1.0;
      else
	return 0.0;
    }
  }

  /**
   * Hinge Loss function
   */
  double HingeLoss_(double yy_hat) {
    if (yy_hat < 1.0)
      return 1.0 - yy_hat;
    else
      return 0.0;
  }
  
  /**
   * Gradient of the Hinge Loss function
   */
  double HingeLossGradient_(double yy_hat) {
    if (yy_hat < 1.0)
      return 1.0;
    else
      return 0.0;
  }

  void LearnersInit_(int learner_typeid);

  int TrainIteration_();

  double GetC_(index_t i) {
    return C_;
  }

  /**
   * Calculate kernel values
   */
  double CalcKernelValue_(index_t i, index_t j) {
    // for SVM_R where n_alpha_==2*n_data_
    if (learner_typeid_ == 1) {
      i = i >= n_data_ ? (i-n_data_) : i;
      j = j >= n_data_ ? (j-n_data_) : j;
    }

    // Check cache
    //if (i == i_cache_ && j == j_cache_) {
    //  return cached_kernel_value_;
    //}

    double *v_i, *v_j;
    v_i = datamatrix_.GetColumnPtr(i);
    v_j = datamatrix_.GetColumnPtr(j);

    // Do Caching. Store the recently caculated kernel values.
    //i_cache_ = i;
    //j_cache_ = j;
    cached_kernel_value_ = kernel_.Eval(v_i, v_j, n_features_);
    return cached_kernel_value_;
  }
};


/**
 * Initialization according to different SVM learner types
 *
 * @param: learner type id 
 */
template<typename TKernel>
void CD<TKernel>::LearnersInit_(int learner_typeid) {
  index_t i;
  learner_typeid_ = learner_typeid;
  
  if (learner_typeid_ == 0) { // SVM_C
    n_alpha_ = n_data_;
    alpha_.Init(n_alpha_);
    alpha_.SetZero();

    w_.Init(n_features_bias_);
    w_.SetZero();

    alpha_.Init(n_alpha_);
    
    coef_.Init(0); // not used, plain init

    y_.Init(n_data_);
    for (i = 0; i < n_data_; i++) {
      y_[i] = datamatrix_.get(datamatrix_.n_rows()-1, i) > 0 ? 1 : -1;
    }
  }
  else if (learner_typeid_ == 1) { // SVM_R
    // TODO
  }
  else if (learner_typeid_ == 2) { // SVM_DE
    // TODO
  }
}


/**
* Coordinate descent based SVM training for 2-classes
*
* @param: input 2-classes data matrix with labels (1,-1) in the last row
*/
template<typename TKernel>
void CD<TKernel>::Train(int learner_typeid, const Dataset* dataset_in) {
  index_t i, j, epo, t, wi;
  int yi;
  double G, C, diff;
  int stopping_condition = 0;
  
  /* general learner-independent initializations */
  dataset_ = dataset_in;
  datamatrix_.Alias(dataset_->matrix());
  n_data_ = datamatrix_.n_cols();
  n_features_ = datamatrix_.n_rows() - 1;
  n_features_bias_ = n_features_ + 1;

  /* learners initialization */
  LearnersInit_(learner_typeid);

  double pgrad; // projected gradient
  double pgrad_max_old = INFINITY;
  double pgrad_min_old = -INFINITY;
  double pgrad_max_new;
  double pgrad_min_new;
  double diag_p = 0.5/ Cp_;
  double diag_n = 0.5/ Cn_;
  double upper_bound_p = INFINITY;
  double upper_bound_n = INFINITY;
  Vector QD;
  QD.Init(n_alpha_);
  QD.SetZero();

  if (regularization_ == 1) { // L1-SVM
    diag_p = 0;
    diag_n = 0;
    upper_bound_p = Cp_;
    upper_bound_n = Cn_;
  }
  
  for (i=0; i< n_alpha_; i++) {
    if (y_[i] >0) {
      QD[i] = diag_p;
    }
    else {
      QD[i] = diag_n;
    }
    for (j=0; j< n_features_; j++) {
      QD[i] = QD[i] + sqrt( datamatrix_.get(j, i) );
    }
    QD[i] = QD[i] + 1;
  }

  if (n_epochs_ > 0) { // # of epochs provided, use it
    n_iter_ = n_data_;
  }
  else { // # of epochs not provided, use n_iter_ to count iterations
    n_epochs_ = 1; // not exactly one epoch, just use it for one loop
  }
  
  ArrayList<index_t> old_from_new;
  old_from_new.Init(n_data_);

  /* Begin CD outer iterations */
  epo = 0;
  while (stopping_condition == 0) {
    /* To mimic the online learning senario, in each epoch, 
       we randomly permutate the training set, indexed by old_from_new */
    for (i=0; i<n_data_; i++) {
      old_from_new[i] = i; 
    }
    for (i=0; i<n_data_; i++) {
      j = rand() % n_data_;
      swap(old_from_new[i], old_from_new[j]);
    }

    pgrad_max_new = -INFINITY;
    pgrad_min_new = INFINITY;

    t = 0;
    /* Begin CD inner iterations */
    for (t=0; t <= n_iter_; t++) {
      wi = old_from_new[t % n_data_];

      Vector xi;
      datamatrix_.MakeColumnVector(wi, &xi);
      xi[n_features_] = 1.0; // for bias term, x <- [x,1], w <- [w, b]
      yi = y_[wi];
      
      G = 0;
      for (j=0; j< n_features_bias_; j++) {
	G += w_[j] * xi[j];
      }
      G = G * yi - 1;

      if (yi > 0) {
	C = upper_bound_p;
	G += alpha_[wi] * diag_p;
      }
      else {
	C = upper_bound_n;
	G += alpha_[wi] * diag_n;
      }
      
      /*
      // TODO: shrinking
      pgrad = 0;
      if (alpha_[wi] == 0) {
	if (G > pgrad_max_old) {
	  //shrinking
	}
	else if (G < 0) {
	  pgrad = G;
	}
      }
      else if (alpha_[wi] == C) {
	if (G < pgrad_min_old) {
	  //shrinking
	}
	else if (G > 0) {
	  pgrad = G;
	}
      }
      else {
	pgrad = G;
      }
      */
      
      if (alpha_[wi] <= CD_ALPHA_ZERO) {
	pgrad = min(G, 0.0);
      }
      else if ( (C-alpha_[wi]) <= CD_ALPHA_ZERO) {
	pgrad = max(G, 0.0);
      }
      else {
	pgrad = G;
      }

      pgrad_max_new = max(pgrad_max_new, pgrad);
      pgrad_min_new = min(pgrad_min_new, pgrad);

      if ( fabs(pgrad)>1.0e-12 ) {
	double alpha_old = alpha_[wi];
	alpha_[wi] = min(  max(alpha_[wi]-G/QD[wi], 0.0), C );
	diff = (alpha_[wi]-alpha_old) * yi;
	for (j=0; j<n_features_bias_; j++) {
	  w_[j] = w_[j] + diff * xi[j];
	}
      }
    } // for t

    // check optimality
    //printf("eps:%d, pgrad_max_new=%lf pgrad_min_new=%lf\n", epo, pgrad_max_new, pgrad_min_new);
    if (pgrad_max_new - pgrad_min_new <= accuracy_) {
      // TODO: unshrinking
      stopping_condition = 1;
      break;
    }
    
    pgrad_max_old = pgrad_max_new;
    pgrad_min_old = pgrad_min_new;
    if (pgrad_max_old <= 0) {
      pgrad_max_old = INFINITY;
    }
    if (pgrad_min_old >= 0) {
      pgrad_min_old = -INFINITY;
    }
    
    epo ++;
    if (epo >= n_epochs_) {
      stopping_condition = 2;
      break;
    }
  } // for epo
  
  if (stopping_condition == 1) {
    printf("CD terminates since the accuracy %f reached !!! Number of epochs run: %d\n", accuracy_, epo);
  }
  else if (stopping_condition == 2) {
    printf("CD terminates since the number of epochs %d reached !!!\n", n_epochs_);
  }
  
  // Calculate objective value; default: not calculation to save time
  int objvalue = fx_param_int(NULL, "objvalue", 0);
  if (objvalue > 0) {
    double v = 0;
    index_t n_sv = 0;
    for (j=0; j<n_features_bias_; j++) {
      v += w_[j];
    }
    for (i=0; i<n_alpha_; i++) {
      if (y_[i] > 0) {
	v += alpha_[i] * (alpha_[i] * diag_p - 2);
      }
      else {
	v += alpha_[i] * (alpha_[i] * diag_n - 2);
      }
      if (alpha_[i] > CD_ALPHA_ZERO) {
	n_sv ++;
      }
    }
    
    printf("Objective value: %lf\n", v);
    printf("Number of SVs: %d\n", n_sv);
  }

}


#endif
