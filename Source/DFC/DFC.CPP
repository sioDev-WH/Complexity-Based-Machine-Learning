/*
	Comlexity-Based Machine Learning Algorithm
	Copyright Synaptics.io 2019
	Author: Wassim Hafez
*/



#include "dfc.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern BOOL exit_flag;
extern BOOL close_flag;   // Updated in overridden CPtDocument::OnCloseDocument()

#define new DEBUG_NEW        // To indicate source line at which memory leaks.

#ifdef _DEBUG
		   CMemoryState oldMemState, newMemState, diffMemState;
#endif

static WORD n_at_top_level;
WORD shift_count;
WORD temp;

// ************************************************************
// Constructors of the logic_function class.
// ************************************************************

logic_function::logic_function(){
   _type = FULL;
   _PC_flag = 0;
   _no_inputs = 0;
   _no_outputs = 0;
   _input_label = NULL;
   _output_label = NULL;
   _table = NULL;   
   _cardinality = 0;

}

logic_function::logic_function(FUN_TYPE    type,
                               int         PC_flag,
                               WORD        no_inputs,
                               WORD        no_outputs,
                               char        **input_label,
                               char        **output_label,
                               BYTE        **table){
   
   WORD  i, table_size_in_bytes;
   WORD  j;
   
                             
   _type = type;
   _PC_flag = PC_flag;
   _no_inputs = no_inputs;
   _no_outputs = no_outputs;
   
   if(input_label != NULL){
      _input_label = (char **) new (char *[_no_inputs]);
      for(i=0; i<_no_inputs;i++){
         _input_label[i] = new char[80];
         if(input_label[i] != NULL)
            strcpy(_input_label[i],input_label[i]);
         else
            strcpy(_input_label[i],"No Label\0");
      }
   }
   else _input_label = NULL;
   
   if(output_label != NULL){
      _output_label = (char**) new (char *[_no_inputs]);
      for(i=0; i<_no_outputs;i++){
         _output_label[i] = (char*)new char[80];
         if(output_label[i] != NULL)
            strcpy(_output_label[i],output_label[i]);
         else
            strcpy(_output_label[i],"No Label\0");
      }
   }
   else _output_label = NULL;
   
   table_size_in_bytes = (~(~0<<1))<<_no_inputs;
   table_size_in_bytes >>= 3*sizeof(BYTE);
   if(table_size_in_bytes == 0)table_size_in_bytes++; //two inputs.
   if(_type == PARTIAL) _table = (BYTE**)new (BYTE *[2*_no_outputs]);
   else _table = (BYTE**)new (BYTE *[_no_outputs]);
   for(j=0;j<_no_outputs;j++){
      _table[j] = (BYTE*)new BYTE[table_size_in_bytes];
      if(table[j] == NULL){
        AfxMessageBox("logic_function constructor::NULL table FULL column",MB_OK,0);
        return; 
      }
      for(i=0;i<table_size_in_bytes;i++) _table[j][i] = table[j][i];
   }
   if(_type == PARTIAL){
      for(j=_no_outputs;j<2*_no_outputs;j++){
         _table[j] = (BYTE*)new (BYTE [table_size_in_bytes]);
         if(table[j] == NULL){
           AfxMessageBox("logic_function constructor::NULL table TBD column",MB_OK,0);
           return; 
         }
         for(i=0;i<table_size_in_bytes;i++) _table[j][i] = table[j][i];
      }
   }
   compute_cardinality();
}

logic_function::logic_function(FUN_TYPE    type,
                               int     PC_flag,
                               WORD    no_inputs,
                               WORD    no_outputs,
                               char    **input_label,
                               char    **output_label,
                               char    *file_name){

   WORD  i, table_size_in_bytes;
   WORD  j;
   BYTE  f;
   FILE *fp;
   _type = type;
   _PC_flag = PC_flag;
   _no_inputs = no_inputs;
   _no_outputs = no_outputs;
 
   
   if(input_label != NULL){
      _input_label = (char **) new (char *[_no_inputs]);
      for(i=0; i<_no_inputs;i++){
         _input_label[i] = (char*)new char[80];
         if(input_label[i] != NULL)
            strcpy(_input_label[i],input_label[i]);
         else
            strcpy(_input_label[i],"No Label\0");
      }
   } 
   else _input_label = NULL;
   
   if(output_label != NULL){
      _output_label = (char**) new (char *[_no_inputs]);
      for(i=0; i<_no_outputs;i++){
         _output_label[i] = (char*)new char[80];
         if(output_label[i] != NULL)
            strcpy(_output_label[i],output_label[i]);
         else
            strcpy(_output_label[i],"No Label\0");
      }
   }
   else _output_label = NULL;
   

   fp = fopen(file_name,"rb");
   
   // Boolean functions are are treated as two bitmaps.
   // The first bitmap is a complete function with arbitrary values
   // assigned to the TBD values; typically 0's. The second bitmap specifies the 
   // TBD values; as 1's at the appropriate locations. The two maps should be
   // specified in the input data file "*.dat", one after the other.
   // Use hexadecimal encoding of the entries. For example, the 2-input
   // XOR function 00 0 01 1 10 1 11 0 file consists of one entry 0x06,
   // which accounts for the function values padded by four 0's; 00000110.
   // The partial function 00 TBD 01 1 10 1 11 0 requires two entries 0x06
   // and 0x01 (alternatively, you can use the pair 0x7 0x1). 
   // *** Notes ***
   // * Entries must be padded, by adding zero at high significant bits.
   // * The entry for 00 is the least significant bit; while that of 11   
   //   is the most significant bit. Therefore, one must be careful while 
   //   reading bytes from bitmaps (cell 000...0 is at top left and 111...1
   //   is at bottom right, corners of the map).
   //   Also, a complete function is made of bytes, with the first read byte is the
   //   least significant one, while the last read byte is the most significant. 
   // * Other encoding schemes may be needed for exetremly long partial
   //   functions.
   
      
   table_size_in_bytes = (~(~0<<1))<<no_inputs;
   table_size_in_bytes >>= 3*sizeof(BYTE);
   if(table_size_in_bytes == 0)table_size_in_bytes++; //two inputs.
   if(_type == PARTIAL) _table = (BYTE **) new (BYTE *[2*no_outputs]);
   else _table = (BYTE **) new (BYTE *[no_outputs]);
   for(j=0;j<no_outputs;j++){
      _table[j] = new BYTE[table_size_in_bytes];
      for(i=0;i<table_size_in_bytes;i++) _table[j][i] = 0;
   }
   if(_type == PARTIAL){
      for(j=no_outputs;j<2*no_outputs;j++){
         _table[j] = new BYTE[table_size_in_bytes];
         for(i=0;i<table_size_in_bytes;i++) _table[j][i] = 0;
      }
   }
   
   for(j=0;j<no_outputs;j++){
      for(i=0;i<table_size_in_bytes;i++){
         if(!fscanf(fp, "%x",&f)){ //Also check on EOF
            AfxMessageBox("Error reading the input file. Use 0X00-0XFF BYTE format.",MB_OK,0);
            return;
         }
         _table[j][i]=f;
      }
   }
   if(_type == PARTIAL){
      for(j=no_outputs;j<2*no_outputs;j++){
         for(i=0;i<table_size_in_bytes;i++){
            if(!fscanf(fp, "%x",&f)){
               AfxMessageBox("Error reading the input file. Use 0X00-0XFF BYTE format.",MB_OK,0);
               return;
            }
            _table[j][i]=f;
         }
      }
   }
   fclose(fp);
   
   compute_cardinality();
 }
 
 
// ************************************************************
// Destructor of the logic_function class.
// ************************************************************

logic_function::~logic_function(){

   WORD i,j,n,m;
   
   n = get_no_inputs();
   m = get_no_outputs();
   
   if(_input_label != NULL){
     for(i=0;i<n;i++){
        if(_input_label[i] != NULL){
           delete _input_label[i];
           _input_label[i] = NULL;
        }
     }
     delete _input_label;
     _input_label = NULL;
   }  
   if(_output_label != NULL){
     for(i=0;i<m;i++){
        if(_output_label[i] != NULL){
           delete _output_label[i];
           _output_label[i] = NULL;
        }
     }
     delete _output_label;
     _output_label = NULL;
   }
   if(_table != NULL){
      for(j=0;j<m;j++){
        if(_table[j] != NULL){
            delete _table[j];
            _table[j] = NULL;
        }
        if(_type == PARTIAL){
           if(_table[j+m] != NULL){
              delete _table[j+m];
              _table[j+m] = NULL;
           }
        }
      }
      delete _table;
      _table = NULL;
   }
}

// To evaluate a function value at given input entry without
// allocating space for the result one must allocate space for
// the result and pass it to the following function, along
// with index for the entry.

STATUS logic_function::evaluate(WORD index , VAL *fun_val){


   WORD         byte_offset;
   WORD		    i,n,m;
   BYTE         bit_offset;
   BYTE         byte;
   VAL          *ptr;
   
   m = get_no_outputs();
   n = get_no_inputs();    // Avoid unnecessary calculations. This function is called too many times.
   if((index < 0) || (index > (WORD)(~(~0<<1)<<n))){ // For example the table upper bound shouldn't be calculated here.
      AfxMessageBox("logic_function:: bound violation in operator[].",MB_OK,0);
      return FAILURE;
   }

   if(fun_val == NULL){
      AfxMessageBox("logic_function:: NULL pointer in operator[].",MB_OK,0);
      return FAILURE;
   } 
   
   byte_offset = index;
   byte_offset>>= 3 ;                  // Divide by sizeof(BYTE).
   bit_offset = (BYTE)(7 & index);     // Remainder of division.
   ptr = fun_val;                                // ptr_fun_val points to first element.
   for(i=0;i<m;i++){
      byte = _table[i][byte_offset];                   // Get byte that contains the value.
      *ptr = (VAL)((byte >> bit_offset) & 1);        // Get bit from byte.
      if(_type == PARTIAL){                            // Check secondary table for TBD.
         byte = _table[i +_no_outputs][byte_offset];   // Get byte that contains the value.
         if((byte >> bit_offset) & 1) *ptr = TBD;  // The TBD bit is ON.
      }
      if(i<m-1)ptr++;
   }    
   return SUCCESS;
}


// ************************************************************

// Cardinality will be defined as the number of specified entries
// in the table. Unspecified entries, although are stored in a
// separate table (as TBD), and used in calculating NU, they are
// not counted in the calculation of the cardinality.
// So, we are calculating logical cardinality as opposed to hardware one.
// In terms of PT reprot, logical cardinality of F is based on NU, while hardware
// cardinality of F is based on [Z] (>= NU). Also, there entries in F which are TBD,
// which are will not be counted either.The logical and hardware
// cardinalities of phi's are the same = log([Z]*(2**s).
 
// So, this approach enables approximation of most complex concepts with
//  simpler interpolations based on the specified entries rather
// than on the number of inputs.
// One needs not wory about complex concepts as long as all its entries are not specified,
// they will be approximated as simple ones. In real life we are not faced with complete
// specifications of complex concepts with large number of inputs, it will take long
// long time to specify them! So we should not consider a concept complex just base on its
// number of inputs. The notion of missing entries play key role in the decomposition
// of functions, since the reduction in DFC of the F function results mainly from missing
// entries for the Z variable of the phi function.

STATUS logic_function::compute_cardinality(){

    WORD no_bytes;
    WORD i,j,k;
    BYTE  mask, byte;
    
    if(_no_inputs > 3)no_bytes = ~(~0<<1) << (_no_inputs-3);      // No bytes/output.
    else no_bytes = 1;
    
    _cardinality = ~(~0<<1) << _no_inputs;
    _cardinality *= _no_outputs; 

    if((_PC_flag == 1)&& (_type == PARTIAL)){
      for(i=_no_outputs;i<2*_no_outputs;i++){
         for(j=0;j<no_bytes;j++){
             byte = _table[i][j];                     // Bytes for TBD entries.
             mask = ~(~0<<1);
             for(k=0;k<8;k++){
               if(mask & byte) _cardinality--;        // Subtract count of TBD entries.
               mask <<= 1;
             }
         }
      }
    }
    
    
    return SUCCESS;
}

WORD logic_function::compute_cardinality(WORD output_no){

    WORD cardinality, no_bytes;
    WORD j,k;
    BYTE  mask, byte;
    
    if(_no_inputs > 3)no_bytes = 1 << (_no_inputs-3);      // No bytes/output.
    else no_bytes = 1;
    
    cardinality = 1 << _no_inputs;


    if((_PC_flag == 1) && (_type == PARTIAL)){
       for(j=0;j<no_bytes;j++){
           byte = _table[output_no+_no_outputs][j];                    // Bytes for TBD entries.
           mask = ~(~0<<1);
           for(k=0;k<8;k++){
             if(mask & byte) cardinality--;        // Subtract count of TBD entries.
             mask <<= 1;
           }
      }
   }

    return cardinality;
}     
       
     
       




// ************************************************************
// Constructors of the decomposed_function class.
// ************************************************************


decomposed_function::decomposed_function(){

    _partition_mask = NULL;
    _phi=NULL;    // Be careful as dynamic decomposition functions may be allocated!
    _F = NULL;
    _phi_flags = NULL;
    _F_flags = NULL;
   _configuration = DISJOINT;
   _strategy = FIRST_SOLUTION;
   _design= FEATURED;
   _random_seed = 1;
   _skip_probability = 0.0;
   _time_limit = 0.0; 
}

decomposed_function::decomposed_function(FUN_TYPE type,
                                         int  PC_flag,
                                         WORD no_inputs,
                                         WORD no_outputs,
                                         char **input_labels,
                                         char **output_labels,
                                         BYTE **table)
                                         :logic_function(type,
                                           PC_flag, 
                                           no_inputs,
                                           no_outputs,
                                           input_labels,
                                           output_labels,
                                           table){
   WORD j;
   shift_count++;
   if(shift_count > 20) 
      TRACE("at (%d,%d) and %d, There are %d allocated functions\n",get_no_inputs(),get_no_outputs(),temp, shift_count);                                      
   _partition_mask = (WORD*) new WORD[get_no_outputs()];
   _phi = (decomposed_function**)new(decomposed_function *[get_no_outputs()]);
   _F   = (decomposed_function**)new(decomposed_function *[get_no_outputs()]);
   _phi_flags = new BYTE[get_no_outputs()];
   _F_flags   = new BYTE[get_no_outputs()];
   for(j=0;j<get_no_outputs();j++){
      _phi_flags[j] = 0;
      _F_flags[j]   = 0;
   }
   _configuration = DISJOINT;
   _strategy = FIRST_SOLUTION;
   _design= FEATURED;
   _random_seed = 1;
   _skip_probability = 0.0;
   _time_limit = 600.0; // Maximum of ten minutes.
}

decomposed_function::decomposed_function(FUN_TYPE type,
                                         int  PC_flag,
                                         WORD no_inputs,
                                         WORD no_outputs,
                                         char **input_labels,
                                         char **output_labels,
                                         char *file_name)
                                         :logic_function(type, 
                                         PC_flag,
                                         no_inputs, 
                                         no_outputs, 
                                         input_labels,
                                         output_labels, 
                                         file_name){
   
   WORD j;                                      
   _partition_mask = (WORD*) new WORD[get_no_outputs()];
   _phi = (decomposed_function**)new(decomposed_function *[get_no_outputs()]);
   _F   = (decomposed_function**)new(decomposed_function *[get_no_outputs()]);
   _phi_flags = new BYTE[get_no_outputs()];
   _F_flags   = new BYTE[get_no_outputs()];
   for(j=0;j<get_no_outputs();j++){
      _phi_flags[j] = 0;
      _F_flags[j]   = 0;
   }
   _configuration = DISJOINT;
   _strategy = FIRST_SOLUTION;
   _design= FEATURED;
   _random_seed = 1;
   _skip_probability = 0.0;
   _time_limit = 600.0; // Maximum of ten minutes.
}






// ************************************************************
// Destructors of the decomposed_function class.
// ************************************************************

decomposed_function::~decomposed_function(){

   WORD j;
   
   
   // for(j=0;j<shift_count;j++) TRACE("\t");
   // TRACE("%d,%d}\n",get_no_inputs(),get_no_outputs());
   shift_count--; 
   if(_partition_mask != NULL){
       delete _partition_mask;
       _partition_mask = NULL;
   }
   for(j=0;j<get_no_outputs();j++){ 
         if(_phi_flags[j] != 0) delete _phi[j];
         if(_F_flags[j] != 0) delete _F[j];
   }
   delete  _phi;
   delete  _F;
   delete _phi_flags;
   delete _F_flags;
   
}






// decomposed_function::operator[] (unsigned int v1, unsigned int v2 )
// calls itself recursively until you call logic_function::operator[] twice?
// You need to use this operator in case the function table was deleted, to
// conserve space. When?







// *************************************************************
// This function is the main engine of the
// application program. It recursively decomposes 
// the function table according to pattern theory.
// ************************************************************ 

WORD decomposed_function::decompose(){
   
   WORD    DFC_phi, DFC_F;            // DFC's of component functions.
   WORD    cardinality, DFC,best_DFC, total_DFC; // Temporary for optimization.
   // WORD    *temp_mask;
   decomposed_function*   phi= NULL;
   decomposed_function*   F=NULL;     // Temporary for optimization.
 
   WORD    n,m,s,j;                   // s is the number of elements in partition V1.
   WORD    ptr;
  
   VAL     *value0,*value1;
   clock_t start, finish;
   double duration;
   int random_val;
   BYTE phi_flag,F_flag;
   
   
   start = clock();
   
   MSG msg;
   BOOL fRetVal = TRUE;
                       
   // AfxCheckMemory();
   // TRACE("Memory check #2\n");
   // ofstream out_file("debug.dat",ios::app);
   
                                   // Initially V1 has two elements and is growing.
   n = get_no_inputs();
   m = get_no_outputs();
   
   if(n_at_top_level < n) n_at_top_level = n;
   // exit_flag = 0;
   // close_flag = 0;
   
   if(n == 1){
     _DFC = 0;
     value0 = new VAL[m];
     value1 = new VAL[m];
     evaluate(0,value0);
     evaluate(1,value1);
     for(j=0;j<m;j++){
        best_DFC = compute_cardinality(j);       // If full then cardinality = 2; else 1.
        if(value0[j] == value1[j]){
          if(value0[j] == TBD) best_DFC = 0;     
          else best_DFC=1;
        }
        _DFC += best_DFC;
     }
     _phi_flags[0] = 0;
     _F_flags[0] = 0;
     
     delete value0;
     delete value1;
     return _DFC;
   }
   
   
   // temp_mask       = new WORD[m];
   
   
   #ifdef _DEBUG
		   if(n==n_at_top_level)oldMemState.Checkpoint();
   #endif
   for(j=0;j<m;j++){
      _partition_mask[j] = 0;
      // temp_mask[j] = 0;
      _phi_flags[j] = 0;
      _F_flags[j] = 0;
   }
   
   
   _DFC = get_cardinality();
   total_DFC = 0;
   // ptr = temp_mask;
   
   
   for(j=0;j<m;j++){

      cardinality = compute_cardinality(j);
      best_DFC = cardinality;
      // _threshold = cardinality;
      ptr = 0;
      phi_flag = 0;
      F_flag = 0;
      // temp_mask[j] = 0;
      
      while(ptr != (WORD)(~(~0<<n-1) << 1)){ 
      
         /* To avoid freezing Windows */
         if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
       
            if(msg.message == WM_QUIT){
            AfxMessageBox("Exit message has been received \n",MB_OK,0);
               fRetVal = FALSE;
               exit_flag = 1;
               goto SAFE_EXIT;
            } 
            /* Does not work for document commands
            if(msg.message == WM_CLOSE){
               AfxMessageBox("Close message has been received \n",MB_OK,0);
               fRetVal = FALSE;
               close_flag = 1;
               goto SAFE_EXIT;
            } 
            */
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
         /*  End of Windows code */    
      
         finish = clock();
         duration = (double) (finish-start) /CLOCKS_PER_SEC; // CLOCKS_PER_SEC = 1000;
         // if(_time_flag == TRUE && duration >= _time_limit) break; 
         s = generate_next_partition(&ptr);
         
         temp = ptr;
         if(_randomization_flag == TRUE){
            random_val = rand();
            if(random_val > ((1-_skip_probability)*32767.0)) continue;    // RAND_MAX = 0x7fff.
         }
         
         if(ptr == 0){
             AfxMessageBox("In decompose::NULL ptr! \n",MB_OK,0);
             return get_cardinality();
         }
         if(test(j, s, &ptr, &phi_flag, &phi, &F_flag, &F) == SUCCESS){    // ensure negative decomposition.
            // TRACE("Memory check #4\n");
            // AfxCheckMemory();
          #ifdef _DEBUG
            if(n==n_at_top_level){
              TRACE("*******\n(%d,%d)\t ptr = %d \t phi=%d F=%d\n",n,m,ptr,phi_flag,F_flag);
		      newMemState.Checkpoint();
		      // if(
		      diffMemState.Difference(oldMemState, newMemState);//){
		         // TRACE("Memory Leak Report I !\n");
		         diffMemState.DumpStatistics();
		         // oldMemState.DumpAllObjectsSince();
		         // return;
		      //}
		    }
		  #endif
            
            
            if(phi_flag != 0){
               DFC_phi = phi->decompose();
               if(exit_flag ||close_flag) goto SAFE_EXIT;
            }      
            else DFC_phi = 0;
            if(F_flag != 0){
                DFC_F   = F->decompose();
                if(exit_flag || close_flag) goto SAFE_EXIT;
            }
            else DFC_F = 0;
            DFC     = DFC_phi + DFC_F;
            
            if(DFC < best_DFC){
               if(_phi_flags[j] != 0)delete _phi[j];
               if(_F_flags[j]   != 0) delete  _F[j];
               if(phi_flag != 0){
                  _phi[j] = phi;
                  _phi_flags[j] = 1;
               }
               else _phi_flags[j] = 0;
               if(F_flag != 0){
                  _F[j]   = F;
                  _F_flags[j] = 1;
               }
               else _F_flags[j] = 0;
               phi_flag = 0;
               F_flag = 0;               
               best_DFC = DFC;
               _partition_mask[j] = ptr;
               if(_strategy == FIRST_SOLUTION){
                  
                  if(n==n_at_top_level){ // If first solution has TBD bits look further
                     int TBD_count = 0;
                     VAL *estimate = new VAL[m];
                     for(WORD bit_index=0;bit_index<(WORD)(~(~0<<1)<<n);bit_index++){
                        extrapolate(bit_index, estimate);
                        for(WORD jj=0;jj<m;jj++)
                          if(estimate[jj] == TBD) TBD_count++;
                     }
                     delete estimate;
                     if(TBD_count == 0) break;
                     else{
                        if(_phi_flags[j] != 0)delete _phi[j];  // If strategy is Best Sol. Save these
                        _phi_flags[j] = 0;		               // in order to restore them and 
               			if(_F_flags[j]   != 0) delete  _F[j];  // the old best_DFC and old part. mask
               			_F_flags[j] = 0;
                        best_DFC = cardinality;    
                        _partition_mask[j] = 0;
                        if(phi_flag != 0){
                            delete phi;
                            phi_flag = 0;
                        }
                        if(F_flag != 0){
                            delete F;
                            F_flag = 0;
                        }
                        continue; 
                     }
                  }
                  break; // Greedy.
               }
            }
            else{
               if(phi_flag != 0){
                  delete phi;
                  phi_flag = 0;
               }
               if(F_flag != 0){
                  delete F;
                  F_flag = 0;
               }
            }
         }             // End of if test(); done with test successfully.
         else{
             // There should be NO phi and F if test is not successful.
             // and you still have phi and F constructed!!
            if((phi_flag != 0) || (F_flag != 0))
               AfxMessageBox("In decompose::Potential Memory Leak \n",MB_OK,0);
             
        }
      }                //End of while (all masks done).
    
      if(best_DFC==cardinality){
         if(phi_flag != 0){ // Shouldn't happen
            AfxMessageBox("In decompose::Potential Memory Leak \n",MB_OK,0);
            delete phi;
            phi_flag = 0;
         }
         if(F_flag != 0){
            AfxMessageBox("In decompose::Potential Memory Leak \n",MB_OK,0);
            delete F;
            F_flag = 0;
         }
         if(_phi_flags[j] != 0){
           delete _phi[j];
           _phi_flags[j] = 0;
         }
         if(_F_flags[j] != 0){
            delete _F[j]; 
            _F_flags[j] = 0;
         }
         total_DFC += cardinality;
      }
      else total_DFC += best_DFC;
      // if(j<m-1)ptr++;
   }                   // End of for{} all outputs.   
   /* if(temp_mask != NULL){
      delete temp_mask;
      temp_mask = NULL;
   }  */
   if(phi_flag != 0) delete phi;
   if(F_flag != 0) delete F;
   _DFC = total_DFC;
   return _DFC;
SAFE_EXIT:
   if(exit_flag || close_flag){
     // delete temp_mask;
      if(phi_flag != 0){ 
            delete phi;
            phi_flag = 0;
      }
      if(F_flag != 0){
            delete F;
            F_flag = 0;
      }
      for(j=0;j<m;j++){
         if(_phi_flags[j] != 0){
           delete _phi[j];
           _phi_flags[j] = 0;
         }
         if(_F_flags[j] != 0){
            delete _F[j]; 
            _F_flags[j] = 0;
         }
      }
      return _DFC;        // Return any value; leaving any way!
   }
   // return _DFC;
}


// **************************************************************
// This function generate disjoint partitions of the input set
// by increasing the number of inputs to the "phi" component.
// The function updates the _partition_mask every time it is called.
// **************************************************************

WORD decomposed_function::generate_next_partition(WORD *partition_mask){
    // search for the most significant bit in the partition mask
    // move that bit to the left, if not the last bit.If it is 
    // the last bit, then set it to zero and move the most significant
    // bit to the left and insert one to the left of it. If you can not
    // insert the one to the left then repeat the process till you can,
    // or increase the number of variables in the mask.
    
    WORD msb, count;
    WORD n,i;
    WORD mask, carry, mask_0, mask_1;
    
   
 
    n = get_no_inputs();
    mask = ~(~0<<1);          // mask initial value is 1.
    mask <<= (n-1);           // mask value is 2**(n-1).
    msb = -1;                // position of msb.
    count = 0;               // how many 1's in current partition?
    

    if(n<1){
       AfxMessageBox("In mask:: #inputs less than 1! \n",MB_OK,0);
       return 0;
    }

    for(i=0;i<n;i++){
       if(mask & (*partition_mask)){ // Found 1 at ith bit.
          count++;
          if(msb == -1) msb = n-1-i;
       }
       mask >>= 1;        // Shift mask to the right.
    }
    
    if(count == 0){                  // Initial partition.
       *partition_mask = ~(~0<<1);   // It had x0 as its
       return 1;                     // its only element.
    }

    carry = 0;
    do{
       if((msb+carry) < n-1){
          mask_0 = ~(~0<<1);
          mask_0 <<= msb; 
          mask_0 = ~mask_0;             // To place 0 at the msb of the partition mask.
          mask_1 = ~(~0<<(carry+1));    // Places 1's at the (carry+1) rmb's.
          mask_1 <<= (msb+1); // To place 1's at the msb+1 of partition.     
          *partition_mask &= mask_0;
          *partition_mask |= mask_1;
          return count;
       }
       else{
          mask_0 = ~(~0<<1);
          mask_0 <<= msb; 
          mask_0 = ~mask_0; // To place 0 at the msb of the partition mask.
          *partition_mask &= mask_0;
          carry++;
          mask = ~(~0<<1);         // mask initial value is 1.
          mask <<= (msb-1);        // mask value is 2**(msb-1); msb>0.
          i = 0;               // To start at the msb-1 bit.
          do{
             if(mask & (*partition_mask)){
               msb = msb-1-i;
               break;
             }
             mask >>= 1;        // Shift mask to the right.
             i++;
          }while(i<msb);
       }
    } while(carry < count);
    
    // We need to increase number of elements.
    if(count == (n-1)) return n; // No more partitions.
    else{
       carry++;                        // Increase number of elements in partition.
       *partition_mask = ~(~0<<carry); // Places 1's in the "carry" 
                                       // right most bits of mask.
       return carry;
    }
}     
       


// Start from original table, (1) test for nontriviality of the current partition,
//                            (2) allocate space for phi and F,
//                            (3) extract phi and F.

      
STATUS decomposed_function::test(WORD r,
                                 WORD s,
                                 WORD *partition_mask,
                                 BYTE *phi_flag,
                                 decomposed_function **phi,
                                 BYTE *F_flag,
                                 decomposed_function **F){
                           
            
   
   WORD i,j,l,t,u;
   WORD n,m,K;
   WORD mask, mask_1, mask_2;
   WORD *col_vars,*row_vars;
   WORD no_columns, no_rows;
   WORD col_index, row_index;
   WORD NU;     // Number of unique columns.
   WORD *z_val, *proto_flag; // each column is assigned a z value (WORD)
                 // These values are exactly the table of phi.
   BYTE **part_table;
   
   WORD no_col_bytes;
   WORD index;
   WORD threshold;
   VAL   *fun_val;
   BYTE bmask, byte, TBD_byte;
   BYTE flag;
   WORD match, compatible;
   BYTE prototype_byte, current_byte;
   char **input_labels,**output_labels;
   char *astring="No Label\0";
   WORD no_F_rows, no_F_columns;
   WORD no_F_col_bytes;
   BYTE **F_table;
   WORD no_phi_rows, no_phi_columns;
   WORD no_phi_col_bytes;
   BYTE **phi_table;
   BYTE *byte_array;
   
   col_vars = NULL;
   row_vars = NULL;
   z_val = NULL;
   part_table = NULL;
   fun_val = NULL;
   F_table = NULL;
   phi_table = NULL;
   byte_array = NULL;
   
   
   
   // ofstream out_file("pt_table.dat",ios::app);
   
   // if(*partition_mask == 1) out_file = new ofstream("pt_table.dat",ios::app);

   // out_fileut_file << "output no = " << (UINT)r << "\tno_col_variables ="<< (UINT)s <<"\n"; 
   
   n = get_no_inputs();
   m = get_no_outputs();
   
   
   

   // First identify column variables based on their
   // number , s, and the partition mask.
    col_vars = new WORD[(int)s];
    row_vars = new WORD[(int)(n-s)];
    mask = ~(~0<<1); 
    l = 0;
    for(i=0;i<n;i++){
       if(mask & (*partition_mask)){ // Found 1 at ith bit.
          if(l<s) col_vars[l++] = i;
          else{
               AfxMessageBox("decomposed_function:: incorrect mask.",MB_OK,0);
               return FAILURE;
          }
       }
       else{
          if((i-l)<(n-s)) row_vars[i-l] = i;
          else{
               AfxMessageBox("decomposed_function:: incorrect mask.",MB_OK,0);
               return FAILURE;
          }
       }
       mask <<= 1;   
    }
    if(s != l){
      AfxMessageBox("decomposed_function:: incorrect mask.",MB_OK,0);
      return FAILURE;
    }
     // out_file << " Col variables are:\t(";
     // for(i=0;i<s;i++)out_file << "\t X" <<col_vars[i];
     // out_file << ")\n";
     // out_file << " Row variables are:\t(";
     // for(i=0;i<n-s;i++)out_file << "\t X" <<row_vars[i];
     // out_file << ")\n";
       
    
    no_columns = (~(~0<<1)) << s;
    no_rows    = (~(~0<<1)) << (n-s);
    no_col_bytes = no_rows >> 3; // 8bits/byte.
    if(no_col_bytes == 0) no_col_bytes=1;
    
    if(get_fun_type() == PARTIAL) part_table = (BYTE**) new (BYTE *[2*no_columns]);
    else part_table = (BYTE**) new (BYTE *[no_columns]);
    for(col_index=0;col_index<no_columns;col_index++){
      part_table[col_index] = (BYTE*) new BYTE[no_col_bytes];
      for(i=0;i<no_col_bytes;i++) part_table[col_index][i] =0;
      if(get_fun_type() == PARTIAL){
         part_table[col_index+no_columns] = (BYTE*) new BYTE[no_col_bytes];
         for(i=0;i<no_col_bytes;i++) part_table[col_index+no_columns][i] =0;
      }
    }
    
     
    fun_val = new VAL[m];
    byte = 0;
    TBD_byte = 0;
    
    for(col_index=0;col_index<no_columns;col_index++){
       // out_file << "\nCol #"<<col_index<<" \t==>\t\t";
       for(row_index=0;row_index<no_rows;row_index++){
       
          index = calculate_index(s,col_index,col_vars,
                                  n-s,row_index,row_vars);
          // value to be inserted in current column
          // @ row_index.But first form byte and TBD_byte
          // every eight tics. When done with a whole table
          // create a decomposed function. This will simplify
          // comparisons between columns (functions with one
          // output), to identify unique columns and form phi
          // and F.
          
          evaluate(index, fun_val);
          
          
          if(fun_val[r] != TBD){  // use the value that corresponds to the rth output.
             if(fun_val[r] == T){
                bmask = ~(~0<<1);
                bmask <<= (row_index&7);
                byte |= bmask;
               // out_file <<"\t1";
             }
             // else out_file <<"\t0";
          } 
          else{
             bmask = ~(~0<<1);
             bmask <<= (row_index&7);
             TBD_byte |= bmask;
             // out_file <<"\t?";
          }
          // If a whole byte was populated then copy it to the current column.
          // Or if the number of rows is less than bits in a byte.
          if(((row_index&7) == 7) || (row_index == (no_rows-1))){
             part_table[col_index][row_index>>3] = byte;
             byte = 0;
             if(get_fun_type() == PARTIAL){
                part_table[col_index+no_columns][row_index>>3] = TBD_byte;
                TBD_byte = 0;
             }
          }

       }
    }
   // out_file << "\n___________________________________________\n";
   // if(*partition_mask == (WORD)(~(~0<<n-1) << 1) && r == m)out_file.close();
    
    
          
    // Now compare values of the part_table for different columns, each
    // columnt represent a value of V1. The distinct columns are assigned
    // unique values which are those of Z.
    
    z_val = (WORD *) new WORD[no_columns];
    proto_flag = (WORD *) new WORD[no_columns];
    
    for(i=0;i<no_columns;i++){
           z_val[i] = -1;      // Markers of unclassified columns.
           proto_flag[i] = 0;      // The flag will be 1 only if column i is a prototype.
    }
    NU = 0;
                                                               
    if(get_fun_type() == FULL){              // In this case one can match byte by byte.
      z_val[0] = NU;
      proto_flag[0] = 1;
      NU++;                         // If NU ends up equals to 1, then col. vars. are
                                    // are irrelevant. Hence the function depends
                                    // only on the row variables.
      for(i=1;i<no_columns;i++){
          for(l=0;l<i;l++){
             if(proto_flag[l] != 1) continue; //Only representatives are considered.
             match = l;
             flag = 1;                  // Assume that col i matches col l.
             for(j=0;j<no_col_bytes;j++){
                if(part_table[i][j] != part_table[l][j]){
                   flag = 0;
                   break;
                }
             }
             if(flag != 0) break;  // match has been found.
          }
          if(flag == 0){           // new distinct column was found. 
             z_val[i] = NU;
             proto_flag[i] = 1;
             NU++;
          } 
          else z_val[i] = z_val[match];
       }
    }
    
    else{  
                                     // First identify columns which are total
                                     // and identify equivalance classes.
       for(i=0;i<no_columns;i++){
          flag = 1;
          for(j=0;j<no_col_bytes;j++){
             if(part_table[i+no_columns][j] != 0){
               flag = 0;
               break;
             }
          }
          if(flag == 1){
             if(NU == 0){ 
                z_val[i] = NU;
                proto_flag[i] = 1;
                NU++;
             }
             else{
                for(l=0;l<i;l++){
                  if(proto_flag[l] != 1) continue; // Do not compare to non-prototype columns.
                  match = l;
                  flag = 1;                   // Assume that col i matches col l.
                  for(j=0;j<no_col_bytes;j++){
                     if(part_table[i][j] != part_table[l][j]){
                        flag = 0;
                        break;
                     }
                  }
                  if(flag != 0) break;  // match has been found.
                }
                if(flag == 0){          // new distinct column was found.
                   z_val[i] = NU;
                   proto_flag[i] = 1; 
                   NU++;
                } 
                else z_val[i] = z_val[match];
             }
          }
       }
       
       // Now find columns that are compatible with prototype columns or with each others.
       for(i=0;i<no_columns;i++){
          if(z_val[i] != -1) continue;    // Skip if prototype or matched already.
          if(NU == 0){                   // In this case (NU == 0) ==> (i==0) too.
             z_val[i] = NU;
             proto_flag[i] = 1;
             NU++;
          }
          else{
             for(l=0;l<no_columns;l++){   // Prototypes can be anywhere.
                if((l==i) || (proto_flag[l] != 1)) continue;           
                compatible = l;
                flag = 1;
                for(j=0;j<no_rows;j++){
                   t= j>>3;
                   u = j&7;
                   current_byte   = part_table[i][t];
                   prototype_byte = part_table[l][t];
                   if(u == 0)byte = ~(~0<<1);
                   else byte <<= 1;                   
                   if((current_byte & byte) != (prototype_byte & byte)){
                      current_byte   = part_table[i+no_columns][t];
                      prototype_byte = part_table[l+no_columns][t];
                      if(((current_byte & byte) == 0) && ((prototype_byte & byte) == 0)){
                         flag = 0;
                         break;
                      }
                   }
                }
                if(flag != 0) break; // Compatible column was found.
             }
             if(flag == 0){
                z_val[i] = NU;
                proto_flag[i] = 1;
                NU++;
             }
             else{
                z_val[i] = z_val[compatible];
                
                // Now unset the TBD bits of the prototype (compatible) if their 
                // corresponding bits in the current column (i) are already unset. 
                // Substitute the TBD value in prototype by that in current column.  
                
                for(j=0;j<no_col_bytes;j++){
                   if(part_table[compatible+no_columns][j] == 0) continue;
                   else{                                        // TBD in prototype.
                      if(part_table[compatible+no_columns][j]
                               != part_table[i+no_columns][j]){ // Mismatch in TBD's
                          prototype_byte = part_table[compatible+no_columns][j];
                          current_byte   = part_table[i+no_columns][j]; 
                          bmask = ~(~0<<1);
                          for(l=0;l<8;l++){
                             if((bmask & prototype_byte) && !(bmask & current_byte)){
                                // Substitute value in prototype by that in current column.
                                part_table[compatible][j] |= bmask & part_table[i][j];
                                part_table[compatible+no_columns][j] &= ~bmask; // Unset TBD flag.
                             }
                             bmask <<= 1;
                          }
                      }
                   }
                }
             }
          }
       }
    }   
    // Now check NU to see if there is negative decomposition. First calculate K.
    // If it is a negative decomposition, then construct phi and F and return SUCESS.
    // otherwise return FAILURE;
    if(NU == 1) K = 0;
    else{
        mask = ~(~0<<1);
        K=0;
        do{K++;} while((mask<<=1), mask < NU);
    }
    
    // Triviality condition.
    flag = 1;               // Success = 1; Failure = 0.
    if(s <= (n-s)) threshold = s;      // Avoid comparing to large number mask_2
    else{
       mask_2 = (~(~0<<1) << (n-s)); 
       if(s <= mask_2) threshold = s;
       else threshold = mask_2;
    }
    double log_NU = log((double)NU)/log(2.0);
    // First compare to NU_LUB. Then if passed triviality test, then check other bounds.
    if( log_NU >= (double) threshold) flag = 0;
    else{                                   
       switch(_design){ 
       
         case THEORETICAL:  // Input function is total.
                             // mask   = ~(~0<<1) << n;
                             mask   = compute_cardinality(r);          
                             mask_1 = ~(~0<<1) << s;           
                             mask_2 = ~(~0<<1) << (n-s);
                             if( (log_NU*mask_1 + NU*mask_2) >= mask)flag = 0;
                             break;
                                                      
         case PHYSICAL:     // Input function is partial. Use cardinality of F for NU*mask2,
                             // which means that the test can be performed after constructing F
                             // or counting no. of TBD entries in the prototype columns.
                             mask   = compute_cardinality(r);          
                             mask_1 = ~(~0<<1) << s;
                             // Commented following 
                             // mask_2 = ~(~0<<1) << (n-s);
                             // if(K*mask_1 + NU*mask_2 >= mask)flag = 0;
                             mask_2 = ~(~0<<1) << (n-s)+K;
                             if((K*mask_1 + mask_2) >= mask)flag = 0;
                             break;
       
         case FEATURED:      
                             if(K > s-1) flag = 0;
                             break;
       }
   }
   
   
   
    if(flag == 0){                                          // This partition does not
                                                            // lead to negative decomposition.
       for(col_index=0;col_index<no_columns;col_index++){
          delete part_table[col_index];
          if(get_fun_type() == PARTIAL)
         delete part_table[col_index+no_columns];
       }
       delete part_table;
    
       delete col_vars;
       delete row_vars;

       delete fun_val;
       delete z_val;
       delete proto_flag;
       
       *phi_flag = 0;
       *F_flag = 0;
        
       return FAILURE;
    }                                  
    
    
    // By ordering inputs to F as { Z(K-1), ... , Z(1), Z(0), X(i(n-1)), ..., X(i(0))}
    // one can simply read the prototype columns as the entries of the F.
    // NB: Check if NU=1, then function phi is constant.
    
    no_F_rows = (~(~0<<1)) << (n-s+K);
    no_F_columns    = 1;
    no_F_col_bytes = no_F_rows >> 3; // 8bits/byte.
    if(no_F_col_bytes == 0) no_F_col_bytes=1;
    if((get_fun_type() == PARTIAL) || (NU < (WORD)(~(~0<<1) << K))){
       F_table = (BYTE**) new (BYTE *[2*no_F_columns]);
       for(col_index=0;col_index<2*no_F_columns;col_index++)
           F_table[col_index] = (BYTE*) new BYTE[no_F_col_bytes];
       for(i=0;i<no_F_col_bytes;i++) F_table[0][i] = 0;
       for(i=0;i<no_F_col_bytes;i++) F_table[1][i] = ~0; // Set all as TBD
       
    }
    else{
       F_table = (BYTE**) new (BYTE *[no_F_columns]);
       for(col_index=0;col_index<no_F_columns;col_index++){
          F_table[col_index] = (BYTE*) new BYTE[no_F_col_bytes];
          for(i=0;i<no_F_col_bytes;i++) F_table[col_index][i] = 0;
       }
    }
    
    
    
    byte = 0;
    TBD_byte = ~0;    
    for(i=0;i<no_columns;i++){
       if(proto_flag[i] != 1) continue;
       l=z_val[i]*no_rows;
       
       for(j=0;j<no_rows;j++){
          // if(l<=NU*no_rows){        // if [z] > NU, there are some TBD values (bytes).
           t=(j+l)>>3;
           u=(j+l)&7;
           mask = ~(~0<<1);
           mask <<=u;  
           if((get_fun_type() == PARTIAL) && (((part_table[i+no_columns][j>>3]&(~(~0<<1)<<(j&7)))==0))) 
                                                                            TBD_byte ^= mask;
           if(part_table[i][j>>3]&(~(~0<<1)<<(j&7))) byte |= mask;
             
           if((((j+l)&7) == 7)||((j+1)==no_rows)){
                  F_table[0][t] |= byte;
                  if(get_fun_type() == PARTIAL) F_table[1][t] &= TBD_byte;  // Initialized TBD
                  byte = 0;
                  TBD_byte = ~0;
           }
       }
    }   
    
    
    // Now the partition table is not needed.
    for(col_index=0;col_index<no_columns;col_index++){
       delete part_table[col_index];
       if(get_fun_type() == PARTIAL)
         delete part_table[col_index+no_columns];
    }
    delete part_table;

    
          
       
    if(NU == 1){
         *phi_flag = 0;
    }
    else{                                      // Notice phi is always full.
       no_phi_rows = (~(~0<<1)) << s;
       no_phi_columns    = K;
       no_phi_col_bytes = no_phi_rows >> 3; // 8bits/byte.
       if(no_phi_col_bytes == 0) no_phi_col_bytes=1;
       phi_table = (BYTE**) new (BYTE *[no_phi_columns]);
       for(col_index=0;col_index<no_phi_columns;col_index++){
         phi_table[col_index] = (BYTE*) new BYTE[no_phi_col_bytes];
         for(i=0;i<no_phi_col_bytes;i++) phi_table[col_index][i] = 0;
       }
       byte_array = new BYTE[K];
       for(j=0;j<K;j++) byte_array[j] = 0;
         
       
       // Transform all z-val's to {0,1,..., NU-1}
       // first by operating on prototypes only. 
       for(i=0;i<no_columns;i++){
          for(j=0;j<K;j++){                          // Decoding the z-val into outputs of phi.
             mask = (~(~0<<1))<<j;
             if((mask & z_val[i]) > 0)               // If jth bit of z_val in current column is ON,
               byte_array[j] |= (~(~0<<1))<<(i&7);   // then set the (i%8)th bit in the (i/8)th byte
                                                     // of the jth column of phi ON. Got it!
          }
          if(((i&7) == 7) || ((i+1) == no_columns)){
             for(j=0;j<K;j++){
                phi_table[j][i>>3] = byte_array[j];
                byte_array[j] = 0;
             }
          }
       }
    }    
    
    // Labels must be assigned to functions's variables before attempting
    // to print the functions. 
    if(NU > 1){ 
        if((get_input_label() != NULL) && (get_output_label() != NULL)){
           input_labels = (char**) new (char *[s]);
           for(i=0;i<s;i++){
             input_labels[i] = new char[80];
             strcpy(input_labels[i],astring);
           }
           output_labels = (char**) new (char *[K]);
           for(i=0;i<K;i++){
              output_labels[i] = new char[80];
              strcpy(output_labels[i],astring);
           }
           *phi = new decomposed_function(FULL,this->get_PC_flag(), s, K, input_labels, output_labels, phi_table);
 
        }
        else *phi = new decomposed_function(FULL,this->get_PC_flag(), s, K, NULL, NULL, phi_table);
  
        if((get_input_label() != NULL) && (get_output_label() != NULL)){ 
           for(i=0;i<s;i++) delete input_labels[i];
           delete input_labels;
           for(i=0;i<K;i++) delete output_labels[i];
           delete output_labels;
        }
        *phi_flag = 1;
        (*phi)->set_design(this->get_design());
        (*phi)->set_configuration(this->get_configuration());
        (*phi)->set_strategy(this->get_strategy());
        (*phi)->set_random_seed(this->get_random_seed());
        (*phi)->set_probability(this->get_probability());
        (*phi)->set_time_limit(this->get_time_limit());
        (*phi)->set_time_flag(this->get_time_flag());
        (*phi)->set_randomization_flag(this->get_randomization_flag());
        
        for(col_index=0;col_index<no_phi_columns;col_index++){
          delete phi_table[col_index];
        }
        delete phi_table; // The constructor has copied it already.
        
    }
    if((get_input_label() != NULL) && (get_output_label() != NULL)){    
       input_labels = (char**) new (char *[K+n-s]);
        for(i=0;i<K+n-s;i++){
           input_labels[i] = new char[80];
           strcpy(input_labels[i],astring);
        }
        output_labels = (char**) new (char *[1]);
        for(i=0;i<1;i++){
           output_labels[i] = new char[80];
           strcpy(output_labels[i],astring);
        }
        if((get_fun_type() == PARTIAL) || (NU < (WORD)(~(~0<<1)<<K)))
         *F  = new decomposed_function(PARTIAL, this->get_PC_flag(),K+n-s, 1, input_labels, output_labels, F_table);
        else *F  = new decomposed_function(FULL,this->get_PC_flag(), K+n-s, 1, input_labels, output_labels, F_table);
    
    }
                
    if((get_fun_type() == PARTIAL) || (NU < (WORD)(~(~0<<1)<<K)))
         *F  = new decomposed_function(PARTIAL, this->get_PC_flag(),K+n-s, 1, NULL,NULL, F_table);
    else *F  = new decomposed_function(FULL,this->get_PC_flag(), K+n-s, 1, NULL,NULL, F_table);
    
    if((get_input_label() != NULL) && (get_output_label() != NULL)){
     for(i=0;i<K+n-s;i++) delete input_labels[i];
        delete input_labels;
     for(i=0;i<1;i++) delete output_labels[i];
        delete output_labels;   
    }
     *F_flag = 1;
    (*F)->set_design(this->get_design());
    (*F)->set_configuration(this->get_configuration());
    (*F)->set_strategy(this->get_strategy());
    (*F)->set_random_seed(this->get_random_seed());
    (*F)->set_probability(this->get_probability());
    (*F)->set_time_limit(this->get_time_limit());
    (*F)->set_time_flag(this->get_time_flag());
    (*F)->set_randomization_flag(this->get_randomization_flag());
    for(col_index=0;col_index<no_F_columns;col_index++){
          delete F_table[col_index];
          if((*F)->get_fun_type() == PARTIAL){
             delete F_table[col_index+no_F_columns];
          }
        }
    delete F_table;
    
    // Clean up
    if(col_vars != NULL) delete col_vars;
    if(row_vars != NULL) delete row_vars;

    if(fun_val != NULL) delete fun_val;
    if(z_val != NULL) delete z_val;
    if(proto_flag != NULL) delete proto_flag;
    if(byte_array != NULL) delete byte_array;
    
    
  return SUCCESS;   
     
 } 
 
 
 
 
 STATUS  decomposed_function::extrapolate(WORD index, VAL *estimate){
 
       WORD j,n,m;
       WORD phi_n,phi_m;
       WORD part_mask;
       WORD phi_index, F_index;
       VAL *phi_value,*F_value;
       VAL *fun_val;
       
       n = get_no_inputs();
       m = get_no_outputs();
       fun_val = new VAL[m];
       
       evaluate(index, fun_val);
       
       for(j=0;j<m;j++){
         if((fun_val[j] != TBD) || ((_phi_flags[j] == 0) && (_F_flags[j] == 0))){
            estimate[j] = fun_val[j];
            continue;
         }
         
         phi_value = NULL;
         F_value = NULL;
         phi_m = 0;
         phi_n = 0;
         part_mask = _partition_mask[j];
         if(_phi_flags[j] != 0){
            phi_n = _phi[j]->get_no_inputs();
            phi_m = _phi[j]->get_no_outputs();
            phi_value = new VAL[phi_m];
            phi_index = get_phi_index(index, n, part_mask);
            _phi[j]->evaluate(phi_index,phi_value);   
         }
         if(_F_flags[j] != 0){
            F_value = new VAL[1];
            F_index = get_F_index(phi_n, phi_m, phi_value, index, n, part_mask);
            if(_F[j]->get_fun_type()==FULL){ 
               _F[j]->evaluate(F_index,F_value);   
            }
            else _F[j]->extrapolate(F_index,F_value);
            estimate[j] = *F_value;
         }
         if(phi_value != NULL){
            delete phi_value;
            phi_value = NULL;
         }
         if(F_value != NULL){
            delete F_value;      
            F_value = NULL;
         }
       }
       delete fun_val;
   return SUCCESS;
 }
 
 
 
 void  decomposed_function::print(ofstream *dstream, ofstream *gstream,WORD level){
   
   WORD i,j,l,n,m;
   WORD mask;
   
   VAL *estimate,*value;
   
 
   
   n = get_no_inputs();
   m = get_no_outputs();
   
   estimate = new VAL[m];
   value    = new VAL[m];
   
   for(l=0;l<level;l++){
      (*dstream)<<"\t";
      (*gstream) <<"\t";
   }
   (*dstream) << "<<<< function >>>>\n";
   for(l=0;l<level;l++) (*dstream)<<"\t";
   (*dstream) << "("<< n <<"," <<m << ")"<<"\n";
   for(l=0;l<level;l++) (*dstream)<<"\t";
    (*dstream) << "DFC = " << get_dfc()<<"\n";
    (*gstream) << "(" << n << "," << m << "," << get_dfc()<<")" <<"\n";
   for(i=0;i<(WORD)(~(~0<<1)<<n);i++){
      for(l=0;l<level;l++) (*dstream)<<"\t";
      if(evaluate(i,value) && extrapolate(i,estimate)){
         mask = 1<<(n-1);
         for(l=0;l<n;l++){
             if((mask & i)>0)  (*dstream)<< "1 ";
             else (*dstream) << "0 ";
             mask >>= 1;
         } 
         (*dstream) << "\t";
         for(j=0;j<m;j++)
           (*dstream) <<"  " << (int)value[j];
         (*dstream) << "\t->\t"; 
         for(j=0;j<m;j++)
           (*dstream) <<"  " << (int)estimate[j];
         (*dstream) << "\n";
      }
      else{
         AfxMessageBox("In DFC.CPP::Error in evaluate() \n",MB_OK,0);
      }
   }
   
   for(j=0;j<m;j++){
      for(l=0;l<level;l++) (*dstream)<<"\t";
      (*dstream) << "Y["<<j<<"]:\n";
      for(l=0;l<level;l++) (*dstream)<<"\t";
      (*dstream) << "Partition: ";
      mask = 1<<(n-1);
      for(i=0;i<n;i++){
         if((mask & (_partition_mask[j]))>0) *dstream << "1 ";
         else (*dstream) << "0 ";
         mask >>= 1;
      }
      (*dstream) << "\n";
      
      if(_phi_flags[j] != 0) _phi[j]->print(dstream,gstream,level+1);
      else{
         for(l=0;l<level+1;l++){
            (*dstream)<<"\t";
            (*gstream)<<"\t";
         }
         (*dstream) << "<< No Phi >>\n";
         (*gstream) << "*\n";
      }
      if(_F_flags[j] != 0) _F[j]->print(dstream,gstream,level+1);
      else{
         for(l=0;l<level+1;l++){
            (*dstream)<<"\t";
            (*gstream)<<"\t";
         }
         (*dstream) << "<< No F   >>\n";
         (*gstream) << "*\n";
      }
      if((_phi_flags[j] == 0) && (_F_flags[j] == 0)){
       for(l=0;l<level+1;l++){
          (*dstream)<<"\t";
          // (*gstream)<<"\t";
       }
            //(*dstream) << "DFC = " << get_dfc()<<"\n";  
            (*dstream) << "DFC = " << compute_cardinality(j)<<"\n";  
            // (*gstream) << get_dfc() <<"\n";
            (*gstream) <<"\n";
      } 
      (*dstream) << "\n";
      (*gstream) << "\n";
   }
   
   delete value;
   delete estimate; 
 }     
 
 // ***************************************************************************
 //  Utility function to compute table index for given column and row indicies.
 // ***************************************************************************
 
WORD calculate_index(WORD s, WORD col_index, WORD *col_vars,
                      WORD t, WORD row_index, WORD *row_vars){
                      
    WORD mask,i;
    WORD index;
    
    mask = ~(~0<<1);
    index = 0;
    for(i=0;i<s;i++){
       if(mask & col_index) index += ~(~0<<1)<<col_vars[i];
       mask <<= 1;
    }
    mask = ~(~0<<1);
    for(i=0;i<t;i++){
       if(mask & row_index) index += ~(~0<<1)<<row_vars[i];
       mask <<= 1;
    }
    return index;
}



 // ***************************************************************************
 //  Utility function to compute index for component function _phi from index.
 // ***************************************************************************
WORD get_phi_index(WORD index,WORD n, WORD part_mask){ 

 	 WORD i, count = 0,result=0;
 	 WORD mask=~(~0<<1);

     for(i=0;i<n;i++){
        if(mask & part_mask){
           if(mask & index)result |= (~(~0<<1) << count);
           count++;
        }
        mask <<=1;
     } 
     
     return result; 
}

 
 // ***************************************************************************
 //  Utility function to compute index for component function _F from index 
 //  and phi_value.
 // ***************************************************************************
WORD get_F_index(WORD phi_n, WORD phi_m, VAL *phi_value, WORD index, WORD n, WORD part_mask){

 	 WORD i, count = 0,result=0;
 	 WORD mask=~(~0<<1);
 	 
     if((phi_value !=NULL) && (phi_n>0) && (phi_m > 0)){
        for(i=0;i<phi_m;i++)
           if(phi_value[i] == T) result |= (~(~0<<1) << i);
        result <<= (n-phi_n); //Place outputs of phi at the end of the WORD.
     }
     for(i=0;i<n;i++){
        if(mask & (~part_mask)){
           if(mask & index)result |= (~(~0<<1) << count);
           count++;
        }
        mask <<=1;
     } 

     return result; 
} 

 
 
 
// *********************************************************
// Temporary debugging function
// *********************************************************
// int find_pattern(decomposed_function *func, SETUP *setup){

decomposed_function* find_pattern(FUN_TYPE 		fun_type,
                  WORD 			no_inputs,
                  WORD			no_outputs,
                  char			**input_labels,
                  char			**output_labels,
                  BYTE			**table, 
                  SETUP			*setup){ 
                  
 // WORD no_inputs,no_outputs;
 // WORD i,j,l;
 // WORD mask;
 VAL *value,*estimate; 
 decomposed_function *func;
 
 int PC_flag = setup->_Cardinality;
 func = new decomposed_function(fun_type,PC_flag, no_inputs,no_outputs,input_labels,output_labels,table);
 
 ofstream out_file("dfc.wri");
 ofstream decompose_stream("decomp.wri",ios::app,filebuf::sh_read|filebuf::sh_write);
 ofstream graph_stream("dfc_grf.txt",ios::app,filebuf::sh_read|filebuf::sh_write);
 
 func->set_configuration(setup->_Configuration);
 func->set_design(setup->_Design);
 func->set_strategy(setup->_Strategy);
 func->set_time_flag(setup->_Time_bound);
 func->set_time_limit(setup->_fSeconds);
 func->set_randomization_flag(setup->_Randomization);
 func->set_probability(setup->_fProbability);
 func->set_random_seed(setup->_nSeed);

 
 no_inputs = func->get_no_inputs();
 no_outputs = func->get_no_outputs();
 value = new VAL[no_outputs];
 estimate = new VAL[no_outputs];
       
 
 func->compute_cardinality();
 out_file << "type = " << (int)func->get_fun_type() << "\n";
 out_file << "number of inputs = " << (int)func->get_no_inputs() << "\n"; 
 out_file << "number of outputs = " << (int)func->get_no_outputs() << "\n";
 out_file << "cardinality = " << (UINT)func->get_cardinality() << "\n";

 if(func->decompose() == 0){
    delete value;
    delete estimate;
    n_at_top_level = 0;
    return 0;
 }
 
  out_file << "DFC = " << func->get_dfc() << "\n";  // We may need to access individual output DFC's.
  /*
  if(AfxMessageBox("Would you like to write the extrapolation results to file.",MB_YESNO,0) == IDYES){
     for(i=0;i<(WORD)(~(~0<<1)<<no_inputs);i++){
      if(func->evaluate(i,value) && func->extrapolate(i,estimate)){
         mask = 1<<(no_inputs-1);
         for(l=0;l<no_inputs;l++){
             if((mask & i)>0) out_file << "1 ";
             else out_file << "0 ";
             mask >>= 1;
         } 
         out_file << "\t";
         for(j=0;j<no_outputs;j++)
           out_file <<"  " << (int)value[j];
         out_file << "\t\t======>\t\t"; 
         for(j=0;j<no_outputs;j++)
           out_file <<"  " << (int)estimate[j];
         out_file << "\n";
      }
      else{
         AfxMessageBox("In DFC.CPP::Error in evaluate() \n",MB_OK,0);
         return 0;
      }
     }
  }
  */
  func->print(&decompose_stream,&graph_stream,0);
  n_at_top_level = 0;
 out_file.close(); 
 delete value;
 delete estimate;
 
 return func;

}


/*
int find_pattern(){

 WORD type;
 WORD no_inputs,no_outputs;
 decomposed_function *func;
 WORD i,j,l,c;
 WORD mask;
 char file_name[80]="\0";
 VAL *value,*estimate; 

 
 
 ifstream header_file("header.dat"); // To be replaced by UI.
 ofstream out_file("dfc.dat");
 
 header_file >> type >> no_inputs >> no_outputs >> file_name;
 
 header_file.close();
 
 value = new VAL[no_outputs];
 estimate = new VAL[no_outputs];
 
 switch(type){
    case 0:
           func = new decomposed_function
             (FULL,no_inputs,no_outputs,NULL,NULL,file_name);
           break;
    case 1:
           func = new decomposed_function
             (PARTIAL,no_inputs,no_outputs,NULL,NULL,file_name);
           break;
    default:
           AfxMessageBox("PT::Illegal function type in header \n",MB_OK,0);
           return 0;
 }
           
           
 
 func->compute_cardinality();
 c = func->get_cardinality();  
 out_file << "type = " << (int)func->get_fun_type() << "\n";
 out_file << "number of inputs = " << (int)func->get_no_inputs() << "\n"; 
 out_file << "number of outputs = " << (int)func->get_no_outputs() << "\n";
 out_file << "cardinality = " << (UINT)c << "\n";

 func->decompose();
 
  out_file << "DFC = " << func->get_dfc() << "\n";  // We may need to access individual output DFC's.
 
 for(i=0;i<(WORD)(~(~0<<1)<<no_inputs);i++){
      if(func->evaluate(i,value) && func->extrapolate(i,estimate)){
         mask = 1<<(no_inputs-1);
         for(l=0;l<no_inputs;l++){
             if((mask & i)>0) out_file << "1 ";
             else out_file << "0 ";
             mask >>= 1;
         } 
         out_file << "\t";
         for(j=0;j<no_outputs;j++)
           out_file <<"  " << (int)value[j];
         out_file << "\t\t======>\t\t"; 
         for(j=0;j<no_outputs;j++)
           out_file <<"  " << (int)estimate[j];
         out_file << "\n";
      }
      else{
         AfxMessageBox("In DFC.CPP::Error in evaluate() \n",MB_OK,0);
         return 0;
      }
 }

 out_file.close(); 
 delete value;
 delete estimate;
 delete func;
 
 return 1;

}
*/