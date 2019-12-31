/*
	Comlexity-Based Machine Learning Algorithm
	Copyright Synaptics.io 2019
	Author: Wassim Hafez
*/

#ifndef __DFC_H__
#define __DFC_H__ 

#include "stdafx.h"
#include <fstream.h>


enum FUN_TYPE {FULL, PARTIAL};

enum STATUS {FAILURE, SUCCESS};

// The following enumeration is used in the current code
// to hold the constant values of the partial boolean function.

enum VAL {F, T, TBD, INVALID};


// The following bit-field representation is an effecient way to handle 
// multi-valued variables; without bit fiddling operators. See K&R pp150.

struct entry {
   unsigned int T   : 1; // Specifies a single bit data field.
   unsigned int TBD : 1; // The compiler will freely squeeze these 
                         // bits into consecutive machine-dependent words.
                         // The direction of bit allocation is not clear.
                         // In Microsoft C/C++ T is first least significant bit.
};
typedef struct entry ENTRY;



// The following bit-field representation is suitable for binary variables
// whose values are always determined, e.g., input variables.

struct binary_entry {
    unsigned int T  : 1;
};
typedef struct binary_entry B_ENTRY;
enum CONFIGURATION {DISJOINT, SHARED};
enum CARDINALITY {FULL_CARDINALITY, PARTIAL_CARDINALITY};
enum DESIGN {PHYSICAL, THEORETICAL, FEATURED};
enum STRATEGY {BEST_SOLUTION, FIRST_SOLUTION, EXHAUSTIVE};

struct setup{

    float 			_fProbability;
	int 			_nSeed;
	double 			_fSeconds;
	CONFIGURATION 	_Configuration;
	CARDINALITY 	_Cardinality;
	DESIGN        	_Design;
	STRATEGY		_Strategy;
	BOOL			_Time_bound;
	BOOL			_Randomization;
};
typedef struct setup SETUP;


// Definitions of the classes


class logic_function{

   FUN_TYPE       _type;          // Complete = 0; else Partial = 1.
   WORD		      _no_inputs;     // WORD is unsigned 16-bit integer.
   WORD           _no_outputs;
   char           **_input_label; // Contains input labels.
   char           **_output_label;// Contains output labels.
   BYTE           **_table;       // BYTE is unsigned 8-bit integer.
                                  // Two column array, one column to hold binary representation
                                  // of function; undefined function entries as encoded as 1's
                                  // in the second column.
   int            _PC_flag;       // How to calculate cardinality (NOT DFC).
   WORD           _cardinality;   // In bits. denotes the number of bits in a single column in _table.
                                  // (UINT 16-bits in Windows 3.0 and later; 32-bits in win32); = size_t.
   
   public:
   logic_function();
   logic_function(FUN_TYPE, int, WORD, WORD, char **, char **, BYTE **);
   logic_function(FUN_TYPE, int, WORD, WORD, char **, char **, char *);
   ~logic_function();
   
   FUN_TYPE      get_fun_type() {return _type;}
   WORD          get_no_inputs() {return _no_inputs;}
   WORD          get_no_outputs() {return _no_outputs;}
   char**		 get_input_label(){return _input_label;}
   char**		 get_output_label(){return _output_label;}
   char*         get_input_label(WORD index) {return _input_label[index];}
   char*         get_output_label(WORD index) {return _output_label[index];}
   STATUS        evaluate(WORD , VAL *);
   void          set_PC_flag(int flag){_PC_flag = flag;}
   int           get_PC_flag(){return _PC_flag;}
   STATUS	     compute_cardinality();
   WORD          compute_cardinality(WORD );  // Computes cardinality of a single output. 
   WORD          get_cardinality() {return _cardinality;}
};
   






// Subclasses can be used to create several
// dialects of decompositions; through virtual
// functions test() and decompose().
   
class decomposed_function: public logic_function {    
   WORD                     *_partition_mask; 
   decomposed_function      **_phi; // Array of pointers to decomposed functions.
   decomposed_function      **_F;
   BYTE						*_phi_flags;
   BYTE						*_F_flags;
   WORD                     _DFC;
   DESIGN					_design;
   CONFIGURATION			_configuration;
   STRATEGY                 _strategy;
   WORD                     _threshold;
   int						_random_seed; // RAND_MAX 0x7fff; 32767?
   float					_skip_probability;
   double				    _time_limit;
   BOOL						_randomization_flag;
   BOOL						_time_flag;
   
   
   public:
   decomposed_function();
   decomposed_function(FUN_TYPE, int, WORD, WORD, char **, char **, BYTE **);
   decomposed_function(FUN_TYPE, int, WORD, WORD, char **, char **, char *);
   ~decomposed_function();
   BYTE				get_phi_flag(WORD index){return _phi_flags[index];}
   BYTE				get_F_flag(WORD index){return _F_flags[index];}
   void    			set_design(DESIGN design){_design = design;}
   DESIGN 			get_design(){return _design;}
   void    			set_configuration(CONFIGURATION configuration){_configuration = configuration;}
   CONFIGURATION  	get_configuration(){return _configuration;}
   void    			set_strategy(STRATEGY strategy){_strategy = strategy;}
   STRATEGY     	get_strategy(){return _strategy;}
   void    			set_threhold(WORD threshold){_threshold = threshold;}
   WORD    			get_threshold() {return _threshold;}
   void    			set_random_seed(int seed){_random_seed = seed;}
   int     			get_random_seed(){return _random_seed;}
   void    			set_probability(float probability){_skip_probability = probability;}
   float  			get_probability(){return _skip_probability;}
   void    			set_time_limit(double limit){_time_limit = limit;}
   double  			get_time_limit(){return _time_limit;}
   void				set_time_flag(BOOL flag){_time_flag = flag;}
   BOOL				get_time_flag(){return _time_flag;}
   void				set_randomization_flag(BOOL flag){_randomization_flag = flag;}
   BOOL				get_randomization_flag(){return _randomization_flag;}
   WORD    			get_dfc() {return _DFC;}
   WORD    			decompose(); // Constructs _phi and _F and returns the DFC of the function.            
   STATUS  			extrapolate(WORD , VAL *); // Equivalent to evaluate but uses recomposition for TBD vals.
   void   			print(ofstream *, ofstream *, WORD);
   
   // private:                                          
   WORD   			generate_next_partition(WORD *); // pass the old mask;
   STATUS   		test(WORD, WORD, WORD *, BYTE *, decomposed_function **,BYTE *, decomposed_function **); // Accepts output no and mask.
};
    


WORD calculate_index(WORD, WORD, WORD *, WORD, WORD, WORD *);
WORD get_phi_index(WORD ,WORD , WORD );
WORD get_F_index(WORD, WORD , VAL*, WORD, WORD, WORD);
// int find_pattern(decomposed_function*, SETUP* );
decomposed_function* find_pattern(FUN_TYPE, WORD, WORD, char **,char **, BYTE **, SETUP *);    
#endif  // __DFC__    
    
    
    
   
   
  