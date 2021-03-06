//~ * Compile with:
//~ g++ -O3 -std=c++11 -o infl_kommuner.exe infl_kommuner_vacc.cpp
//~ * ./infl_kommuner_vacc.exe 
//~ * */
// #include "stdafx.h"
#include "infl_kommuner_vacc.h"
using namespace std;
default_random_engine generator;

int random_binomial(int n, double p){
    binomial_distribution<int> distribution(n,p);
    return distribution(generator);
}

void cumulative_sum(double **outarray, double **array, int n){
	/// REMEMBER to delete outarray after use
	(*outarray)[0] = (*array)[0];
	for(int i=1; i < n; ++i){
		(*outarray)[i] = (*outarray)[i-1] + (*array)[i];
	}
}

void seir_sim(int &ds, int &de1, int &de2, int &dia, int &di,int &dsvi, int &dve1, int &dve2, int &dvia, int &dvi,       // Outputs
            int S, int E, int Ia, int I, int SVI, int VE, int VIa, int VI, double beta, double a,  double gamma, int pop, double delta_t){ // Inputs
     // Function to run one time step of the seir model
    // ds are susceptible going to exposed
    // de1 are the exposed going to asymptomatic,
    // de2 are the exposed going to symptomatic,  
    // dia are the infectious asymptomatic going to recovered
    // di are the infectious symptomatic going to recovered 
    // dsvi are the susceptible vaccinated going to exposed
    // dve1 are the exposed vaccinated going to asymptomatic
    // dve2 are the exposed vaccinated going to symptomatic  
    // dvia are the vaccinated asymptomatic infectious going to recovered
    // dvi are the vaccinated symptomatic infectious going to recovered
    ds = 0; de1 = 0; de2 = 0; dia = 0; di = 0;  dsvi = 0; dve1 = 0; dve2 = 0; dvia = 0; dvi = 0;
    int dve = 0;	
	
	float vr = 0.2; // Relative infectiousness of the unsuccessfully vaccinated. 
	    
    
    int de = 0;
    if(I != 0 || Ia != 0 || E != 0 || VI != 0 || VE != 0 || VIa != 0){
        if( E == 0){
            de = 0;
        }
        else{
            de = random_binomial(E, a*delta_t);
            if(de != 0){
                de1 = random_binomial(de, 0.33);
                de2 = de-de1;
            }
        }
        if(I == 0){
            di = 0;
        }
        else{
            di = random_binomial(I, gamma*delta_t);
        }
        if(Ia == 0){
            dia = 0;
        }
        else{
            dia = random_binomial(Ia, gamma*delta_t);
        }
        if (VI == 0){
			dvi = 0;
		}
		else{
			dvi = random_binomial(VI, gamma*delta_t);
		}		
        if (VIa == 0){
			dvia = 0;
		}
		else{
			dvia = random_binomial(VIa, gamma*delta_t);
		}	
		if (VE == 0){
			dve = 0;
		}
        else{
            dve = random_binomial(VE, a*delta_t);
            if(dve != 0){
                dve1 = random_binomial(dve, 0.33);
                dve2 = dve-dve1;
            }
        }		
		ds = random_binomial(S, beta*delta_t*I/pop + 0.5*beta*delta_t*Ia/pop + vr*beta*delta_t*VI/pop + vr*0.5*beta*delta_t*VIa/pop);
		dsvi = random_binomial(SVI, beta*delta_t*I/pop + 0.5*beta*delta_t*Ia/pop +vr*beta*delta_t*VI/pop + vr*0.5*beta*delta_t*VIa/pop);
    }
}



Location::Location(string name_, int Shome){
		name = name_;
		S = Shome;
		E = 0; 
		I = 0;
		Ia = 0;
		R = 0;
		N = 0;
		V = 0;
		VI = 0;
		VE = 0;
		VIa = 0;
		SVI = 0;
}

void Location::add_inlink(Link *link){
	// This function adds a pointer to a newly created link that points to this location
	in_links.push_back(link);
}

void Location::add_outlink(Link  *link){
	// This function adds a pointer to a newly created link that points from this location
	out_links.push_back(link);
	
}

void Location::print(){
	cout << "Location " << name << " with S= "<< S << ", E=" << E <<", I=" << I << ", Ia=" << Ia << ", R= " << R << ". " << endl;
	cout << "out_links: "<< out_links.size() <<endl;
	for(int i = 0; i < out_links.size(); ++i){
		cout << "Trying to print from out_link " << i << endl;
		out_links[i]->print();
		cout.flush();
	}
	cout << "in_links: "<< in_links.size()<< endl;
	for(int i = 0; i < in_links.size(); ++i){
		cout << "Trying to print from in_link " << i << endl;
		in_links[i]->print();
		cout.flush();
	}
	
}
	
void Location::seir_step_day(float beta, float a, float gamma, int &de2, int &dve2){
	// Function to run a day time step of the model. 
	// The commuters are all sent to their work locations. 	
	// Return symptomatic incidence, de2 and dve2
	int S_tmp = S;
	int E_tmp = E;
	int Ia_tmp = Ia;
	int I_tmp = I;
	int R_tmp = R;
	int V_tmp = V; 
	int VI_tmp = VI;
	int VE_tmp = VE;
	int VIa_tmp = VIa;
	int SVI_tmp = SVI;
	double pop_tmp = S + E + Ia + I + R + V + VI + SVI + VE + VIa;
	int num = in_links.size();
	
	//Vectors with the number of people in the respective compartment, 
	//on each link, and in the home population
	//used to distribute the transitions between compartments 
	//between the commuters on the different links and the home population
	//The in-edges are on the first num elements, the home population last
	int *S_probs = new int[num+1]; 
	int *E_probs = new int[num+1];
	int *I_probs = new int[num+1];
	int *Ia_probs = new int[num+1];
	int *R_probs = new int[num+1];
	int *V_probs = new int[num+1];
	int *VI_probs = new int[num+1];	
	int *VIa_probs = new int[num+1];
	int *VE_probs = new int[num+1];	
	int *SVI_probs = new int[num+1];
	for(int i = 0; i < in_links.size(); ++i){
		S_tmp += in_links[i]->S;
		E_tmp += in_links[i]->E;
		Ia_tmp += in_links[i]->Ia;
		I_tmp += in_links[i]->I;
		R_tmp += in_links[i]->R;
		V_tmp += in_links[i]->V;
		VI_tmp += in_links[i]->VI;
		SVI_tmp += in_links[i]->SVI;
		VIa_tmp += in_links[i]->VIa;
		VE_tmp += in_links[i]->VE;
		pop_tmp += in_links[i]->S + in_links[i]->E + in_links[i]->Ia + in_links[i]->I + in_links[i]->R + in_links[i]->V + in_links[i]->VI + in_links[i]->VE + in_links[i]-> VIa + in_links[i]->SVI;
	
		S_probs[i] = in_links[i]->S;
		I_probs[i] = in_links[i]->I;
		Ia_probs[i] = in_links[i]->Ia;
		E_probs[i] = in_links[i]->E;
		R_probs[i] = in_links[i]->R;
		V_probs[i] = in_links[i]->V;
		VI_probs[i] = in_links[i]->VI; 
		VIa_probs[i] = in_links[i]->VIa; 
		VE_probs[i] = in_links[i]->VE; 
		SVI_probs[i] = in_links[i]->SVI; 
	}	

	S_probs[num] = S;
	I_probs[num] = I;
	Ia_probs[num] = Ia;
	E_probs[num] = E;
	R_probs[num] = R;
	V_probs[num] = V;
	VI_probs[num] = VI;
	VIa_probs[num] = VIa;
	VE_probs[num] = VE;
	SVI_probs[num] = SVI;
	int ds; int de1; int dia; int di; int dvi; int dsvi; int dvia; int dve1; 

	// Run the SEIR step
	seir_sim(ds, de1, de2, dia, di, dsvi, dve1, dve2, dvia, dvi, S_tmp, E_tmp, Ia_tmp, I_tmp, SVI_tmp, VE_tmp, VIa_tmp, VI_tmp, beta, a, gamma, pop_tmp, 12.0/24.0);	

	//Distribute the transitions
	double *probs = new double[num+1];
	double *probs_cum = new double[num+1];
	double randomnumber;
	int index = -1;
	for(int i = 0; i < dvi; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VI_probs[k]*1.0/ VI_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}	
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){	
			// Add to home pop
			VI -= 1;
			R += 1;
		}
		else if(index < in_links.size()){						
			in_links[index]->VI -= 1;
			in_links[index]->R += 1;
		}
	
		VI_probs[index] -= 1;
		VI_tmp -= 1;
	
	}	
	
	for(int i = 0; i < dvia; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VIa_probs[k]*1.0/ VIa_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){	
			// Add to home pop
			VIa -= 1;
			R += 1;
		}
		else if(index < in_links.size()){						
			in_links[index]->VIa -= 1;
			in_links[index]->R += 1;
		}	
	
		VIa_probs[index] -= 1;
		VIa_tmp -= 1;
	
	}

	for(int i = 0; i < dve1; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VE_probs[k]*1.0/ VE_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VE -= 1;
			VIa += 1;
		}
		else if(index < in_links.size()){				
			in_links[index]->VE -= 1;
			in_links[index]->VIa += 1;
		}			
		
		VE_probs[index] -= 1;
		VE_tmp -= 1;
	
	}	

		
	for(int i = 0; i < dve2; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VE_probs[k]*1.0/ VE_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VE -= 1;
			VI += 1;
		}
		else if(index < in_links.size()){					
			in_links[index]->VE -= 1;
			in_links[index]->VI += 1;
		}					
	
		VE_probs[index] -= 1;
		VE_tmp -= 1;
	
	}		


	for(int i = 0; i < dsvi; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =SVI_probs[k]*1.0/SVI_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}	
		cumulative_sum(&probs_cum, &probs, num+1);				
		for(int h = 0; h < num+1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}
		if(index == num){
			// Add to home pop				
			SVI -= 1;
			VE += 1;
		}
		else if(index < in_links.size()){							
			in_links[index]->SVI -= 1;
			in_links[index]->VE += 1;
		}
		SVI_probs[index] -= 1;
		SVI_tmp -= 1;
		
	}
	//Not vaccinated	
	for(int i = 0; i < dia; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =Ia_probs[k]*1.0/ Ia_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){	
			// Add to home pop
			Ia -= 1;
			R += 1;
		}
		else if(index < in_links.size()){						
			in_links[index]->Ia -= 1;
			in_links[index]->R += 1;
		}
	
		Ia_probs[index] -= 1;
		Ia_tmp -= 1;
	
	}

	for(int i = 0; i < di; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =I_probs[k]*1.0/ I_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			I -= 1;
			R += 1;
		}
		else if(index < in_links.size()){					
			in_links[index]->I -= 1;
			in_links[index]->R += 1;
		}	
	
		I_probs[index] -= 1;
		I_tmp -= 1;
	
	}


	for(int i = 0; i < de1; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =E_probs[k]*1.0/ E_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			E -= 1;
			Ia += 1;
		}
		else if(index < in_links.size()){				
			in_links[index]->E -= 1;
			in_links[index]->Ia += 1;
		}				
		
		E_probs[index] -= 1;
		E_tmp -= 1;
	
	}	
		
	for(int i = 0; i < de2; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =E_probs[k]*1.0/ E_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			E -= 1;
			I += 1;
		}
		else if(index < in_links.size()){					
			in_links[index]->E -= 1;
			in_links[index]->I += 1;
		}						
		E_probs[index] -= 1;
		E_tmp -= 1;
	
	}		


	for(int i = 0; i < ds; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =S_probs[k]*1.0/S_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}	
		cumulative_sum(&probs_cum, &probs, num+1);				
		for(int h = 0; h < num+1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}
		if(index == num){
			// Add to home pop				
			S -= 1;
			E += 1;
		}
		else if(index < in_links.size()){							
			in_links[index]->S -= 1;
			in_links[index]->E += 1;
		}
		S_probs[index] -= 1;
		S_tmp -= 1;
		
	}
	delete[] probs;
	delete[] probs_cum;
	delete[] S_probs;
	delete[] E_probs;
	delete[] I_probs;
	delete[] Ia_probs;
	delete[] R_probs;
	delete[] V_probs;
	delete[] VI_probs;
	delete[] VIa_probs;
	delete[] VE_probs;
	delete[] SVI_probs;
}

void Location::seir_step_night(float beta, float a, float gamma, int &de2, int &dve2){
	// Function to run a night time step of the model. 
	// The commuters are all sent to their home locations.
	// Return symptomatic incidence, de2 and dve2
	int S_tmp = S;
	int E_tmp = E;
	int Ia_tmp = Ia;
	int I_tmp = I;
	int R_tmp = R;
	int V_tmp = V; 
	int VI_tmp = VI;
	int VE_tmp = VE;
	int VIa_tmp = VIa;
	int SVI_tmp = SVI;
	double pop_tmp = S + E + Ia + I + R + V + VI + SVI + VE + VIa;
	int num = out_links.size();
	//Vectors with the number of people in the respective compartment, 
	//on each link, and in the home population
	//used to distribute the transitions between compartments 
	//between the commuters on the different links and the home population
	//The out-edges are on the first num elements, the home population last
	int *S_probs = new int[num+1]; 
	int *E_probs = new int[num+1];
	int *I_probs = new int[num+1];
	int *Ia_probs = new int[num+1];
	int *R_probs = new int[num+1];
	int *V_probs = new int[num+1];
	int *VI_probs = new int[num+1];	
	int *VIa_probs = new int[num+1];
	int *VE_probs = new int[num+1];	
	int *SVI_probs = new int[num+1];	
	for(int i = 0; i < out_links.size(); ++i){	
		S_tmp += out_links[i]->S;
		E_tmp += out_links[i]->E;
		Ia_tmp += out_links[i]->Ia;
		I_tmp += out_links[i]->I;
		R_tmp += out_links[i]->R;
		V_tmp += out_links[i]->V;
		VI_tmp += out_links[i]->VI;
		SVI_tmp += out_links[i]->SVI;
		VIa_tmp += out_links[i]->VIa;
		VE_tmp += out_links[i]->VE;		
		pop_tmp += out_links[i]->S + out_links[i]->E + out_links[i]->Ia + out_links[i]->I + out_links[i]->R + out_links[i]->V + out_links[i] -> SVI + out_links[i] -> VI + out_links[i]-> VIa + out_links[i] -> VE;
		
		S_probs[i] = out_links[i]->S;
		I_probs[i] = out_links[i]->I;
		Ia_probs[i] = out_links[i]->Ia;
		E_probs[i] = out_links[i]->E;
		R_probs[i] = out_links[i]->R;
		V_probs[i] = out_links[i] -> V;
		VI_probs[i] = out_links[i] -> VI;
		VE_probs[i] = out_links[i] -> VE;
		SVI_probs[i] = out_links[i] -> SVI;
		VIa_probs[i] = out_links[i] -> VIa;
	}
	
	S_probs[num] = S;
	I_probs[num] = I;
	Ia_probs[num] = Ia;
	E_probs[num] = E;
	R_probs[num] = R;
	SVI_probs[num] = SVI;
	VI_probs[num] = VI;
	VIa_probs[num] = VIa;
	VE_probs[num] = VE;
	V_probs[num] = V;
	int ds; int de1; int dia; int di; int dvi; int dsvi; int dvia; int dve1;
	
	// Run the SEIR step
	seir_sim(ds, de1, de2, dia, di, dsvi, dve1, dve2, dvia, dvi, S_tmp, E_tmp, Ia_tmp, I_tmp, SVI_tmp, VE_tmp, VIa_tmp, VI_tmp, beta, a, gamma, pop_tmp, 12.0/24.0);	

	//Distribute the transitions
	double *probs = new double[num+1];
	double *probs_cum = new double[num+1];
	double randomnumber;
	int index = -1;
	for(int i = 0; i < dvia; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VIa_probs[k]*1.0/ VIa_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VIa -= 1;
			R += 1;
		}
		else if(index < out_links.size()){	
			out_links[index]->VIa -= 1;
			out_links[index]->R += 1;
		}

		VIa_probs[index] -= 1;
		VIa_tmp -= 1;
		
	}

	for(int i = 0; i < dvi; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VI_probs[k]*1.0/ VI_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}		
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VI -= 1;
			R += 1;
		}
		else if(index < out_links.size()){			
			out_links[index]->VI -= 1;
			out_links[index]->R += 1;
		}
		
		
		VI_probs[index] -= 1;
		VI_tmp -= 1;
		
	}
	
	for(int i = 0; i < dve1; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VE_probs[k]*1.0/ VE_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VE -= 1;
			VIa += 1;
		}
		else if(index < out_links.size()){						
			out_links[index]->VE -= 1;
			out_links[index]->VIa += 1;
		}
	
		
		VE_probs[index] -= 1;
		VE_tmp -= 1;
		
	}	
	
	for(int i = 0; i < dve2; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =VE_probs[k]*1.0/ VE_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			VE -= 1;
			VI += 1;
		}
		else if(index < out_links.size()){					
			out_links[index]->VE -= 1;
			out_links[index]->VI += 1;
		}
		VE_probs[index] -= 1;
		VE_tmp -= 1;
		
	}		

	for(int i = 0; i < dsvi; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =SVI_probs[k]*1.0/ SVI_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}		
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}		
					
		if(index == num){
			// Add to home pop		
			SVI -= 1;
			VE += 1;
		}
		else if(index < out_links.size()){					
			out_links[index]->SVI -= 1;
			out_links[index]->VE += 1;
		}
		
		SVI_probs[index] -= 1;
		SVI_tmp -= 1;
		
	}
	//Without vaccine	
	for(int i = 0; i < dia; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =Ia_probs[k]*1.0/ Ia_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			Ia -= 1;
			R += 1;
		}
		else if(index < out_links.size()){	
			out_links[index]->Ia -= 1;
			out_links[index]->R += 1;
		}		
		
		Ia_probs[index] -= 1;
		Ia_tmp -= 1;
		
	}

	for(int i = 0; i < di; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =I_probs[k]*1.0/ I_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}		
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			I -= 1;
			R += 1;
		}
		else if(index < out_links.size()){			
			out_links[index]->I -= 1;
			out_links[index]->R += 1;
		}
		
		
		I_probs[index] -= 1;
		I_tmp -= 1;
		
	}
	
	for(int i = 0; i < de1; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =E_probs[k]*1.0/ E_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}		
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			E -= 1;
			Ia += 1;
		}
		else if(index < out_links.size()){						
			out_links[index]->E -= 1;
			out_links[index]->Ia += 1;
		}
		
		E_probs[index] -= 1;
		E_tmp -= 1;
		
	}	
	
	for(int i = 0; i < de2; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =E_probs[k]*1.0/ E_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}			
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}			
		if(index == num){
			// Add to home pop
			E -= 1;
			I += 1;
		}
		else if(index < out_links.size()){					
			out_links[index]->E -= 1;
			out_links[index]->I += 1;
		}	
		E_probs[index] -= 1;
		E_tmp -= 1;
		
	}		

	for(int i = 0; i < ds; ++i){
		for(int k = 0; k < num+1; ++k) probs[k] =S_probs[k]*1.0/ S_tmp; // Oneline for-loop
		randomnumber = double(rand())/RAND_MAX;
		while(randomnumber == 0 || randomnumber == 1){
			randomnumber = double(rand())/RAND_MAX;
		}	
		cumulative_sum(&probs_cum, &probs, num+1);
		for(int h = 0; h < num + 1; ++h){
			if(randomnumber < probs_cum[h]){
				index = h;
				break;
			}	
		}		
					
		if(index == num){
			// Add to home pop		
			S -= 1;
			E += 1;
		}
		else if(index < out_links.size()){					
			out_links[index]->S -= 1;
			out_links[index]->E += 1;
		}
		
		S_probs[index] -= 1;
		S_tmp -= 1;
		
	}	
	
	delete[] probs;
	delete[] probs_cum;
	
	delete[] S_probs;
	delete[] E_probs;
	delete[] I_probs;
	delete[] Ia_probs;
	delete[] R_probs;
	delete[] SVI_probs;
	delete[] VI_probs;
	delete[] VE_probs;
	delete[] VIa_probs;
	delete[] V_probs;
}


Link::Link(Location *from_, Location *to_,int S_, int E_, int I_, int Ia_, int R_, int V_, int VI_, int SVI_, int VIa_, int VE_){
		from = from_;
		to = to_;
		S  = S_;
		E  = E_;
		I  = I_;
		Ia = Ia_;
		R  = R_;
		V = V_;
		VI = VI_;
		SVI = SVI_;
		VIa = VIa_;
		VE = VE_;	
}

void Link::print(){
	cout << "Link from " << from->name << ", to " << to->name << ", with S= "<< S << ", E=" << E <<", I=" << I << ", Ia=" << Ia << ", R= " << R << endl;	
}


Graph::Graph(){
}
	
void Graph::add_node(string name, int Shome){
	Location newlocation(name, Shome);
	locations.push_back(newlocation);
}

void Graph::add_edge(string name1, string name2, int S, int E, int I, int Ia, int R, int V, int VI, int VIa, int SVI, int VE){
	/// Find locations with names name1 and name2
	int name1_index = -1;
	int name2_index = -1;
	for (int i = 0; i < locations.size(); ++i){
		if(locations[i].name.compare(name1) == 0){
			name1_index = i;
		}
		if(locations[i].name.compare(name2) == 0){
			name2_index = i;
		}
	}
	if( ((name1_index == -1) || (name2_index == -1)) || (name1_index == name2_index)){
		cout << "Error in add edge with input: " << name1 << ", " << name2 << ", " << S << ", " << E << ", " << I << ", " << Ia << ", " << R << endl;
		cout << "name1_index = " << name1_index << ", name2_index = " << name2_index << endl;
		exit(1);
	}
	Link newlink(&(locations[name1_index]), &(locations[name2_index]), S, E, I, Ia, R, V, VI, VIa, SVI, VE);
	newlink.from_index = name1_index;
	newlink.to_index = name2_index; 
	edges.push_back(newlink);	
}


void Graph::add_edge_index(int i1, int i2, int S, int E, int I, int Ia, int R, int V, int VI, int VIa, int SVI, int VE){
	Link newlink(&(locations[i1]), &(locations[i2]), S, E, I, Ia, R, V, VI, VIa, SVI, VE);
	edges.push_back(newlink);	
}


void Graph::inform_locations_of_edges(){
	for (vector<Link>::iterator it = edges.begin() ; it != edges.end(); ++it){
		(*it).from->add_outlink(&(*it));
		(*it).to->add_inlink(&(*it));
	}
}

void Graph::print(){
	cout << endl << "Printing graph: " << endl;
	cout << "Links: " << endl;
	for(int i = 0; i < edges.size(); ++i){
		edges[i].print();
	}
	cout << "Locations: " << endl;
	for(int i = 0; i < locations.size(); ++i){
		locations[i].print();
		cout.flush();
	}
	
	cout << endl << endl;;
}

void Graph::copy_graph(Graph G){
	for(vector<Location>::iterator it = G.locations.begin() ; it != G.locations.end(); ++it){
		add_node(it->name, it->S);
	}
	for(vector<Link>::iterator it = G.edges.begin() ; it != G.edges.end(); ++it){
		add_edge_index(it->from_index, it->to_index, it->S, it->E, it->I, it->Ia, it->R, it->V, it->VI, it-> VIa, it-> SVI, it-> VE);
	}
	inform_locations_of_edges();
}



int main(int nargs, char ** argsv){
	float beta;  // infection parameter, 0.6
	float gamma; // 1/infectious period, 1/3
	float a;  // 1/latent period, 1/1.9
	int N = 1; // Number of repetitions
	int M = 300; // Number of days
	int n=0; // Number of locations 

	
	beta = atof(argsv[1]);
	a = 1/atof(argsv[2]);
	gamma = 1/atof(argsv[3]);
	Graph G; 
	string tmpstr;
	string tmpstr2;
	char tmpchar[3];
	int pop;
	// Read node file with the names and population sizes without commuters 
	ifstream infile("pop_wo_com.txt");
	if(!infile.is_open()){
		cout << "Error in opening network node file" << endl;
		exit(1);
	}
	char name[256];
	char name2[256];
	while(infile  >> tmpstr >> pop){
		G.add_node(tmpstr, pop);
		n+= 1;
	}
	infile.close();
	
	int S, E, I, Ia, R;
	E = 0;
	I = 0;
	Ia = 0;
	R = 0;
	// Read edges file with the number of commuters
	//Should not include edges with zeros for computational efficiency
	ifstream edge_infile("di_edge_list.txt");
	if(!edge_infile.is_open()){
		cout << "Error in opening network edge file" << endl;
		exit(1);
	}
	int safecount = 0;
	int n_edges = 0;
	cout << "Starting to add edges, printing every 1000 edge" << endl;
	while(edge_infile >> tmpstr >> tmpstr2 >> S){
		if (S != 0){
			safecount += 1;
			G.add_edge(tmpstr, tmpstr2, S, E, I, Ia, R, 0, 0, 0, 0, 0);
		
			if(safecount % 1000 == 0){
				cout << safecount << " ";
				cout.flush();
			}
			if(edge_infile.eof()){
				break;
			}
			n_edges += 1;
		}
	}	
	cout << "Found " << n_edges << " edges. " << endl;
	cout << endl << endl;
	edge_infile.close();
	G.inform_locations_of_edges();
	// Storage for statistics
	
	//The state in each location at each time point
	int ***values = new int**[n]; 
	
	// Peak dates in each location
	int **peak_date = new int*[n]; 
	
	// Peak number infected in each location
	int **peak_val = new int*[n]; 
	
	//Initial dates in each location
	//defined as first day where the symptomatic prevalence has been 
	//more than 1% for 7 consecutive days 
	int **start_date = new int*[n]; 
	
	//Dummy vector, to find peak and initial dates
	int **I_this = new int*[n];
	
	//Final number infected in each location
	int **final_size = new int*[n];
	
	//Save the prevalence curves (infectious + infectious asymptomatic) for each location
	// For each run, in order to make confidence curves over the N simulations
	int **bonds = new int*[N];
	
	/// Values has first index for position, second for time and third for value.
	/// Third index 0=S, 1=E, 2=I, 3=Ia, 4=R; 5 = VI, 6 = VIa, 7 = incidence symptomatic;
	for(int i = 0; i < n; ++i){
		values[i] = new int*[M*2]; // 2*M, stored for both day and night time.
		peak_date[i] = new int[N];
		peak_val[i] = new int[N];
		start_date[i] = new int[N];
		I_this[i] = new int[M*2];
		final_size[i] = new int[N];
		for(int k = 0; k < M*2; ++k){
			values[i][k] = new int[8];
			values[i][k][0] = 0;
			values[i][k][1] = 0;
			values[i][k][2] = 0;
			values[i][k][3] = 0;
			values[i][k][4] = 0;
			values[i][k][5] = 0;
			values[i][k][6] = 0;
			values[i][k][7] = 0;
		}
	}
	
	for (int i = 0; i < N; ++i){
		bonds[i] = new int[2*M]; // 2*M, stored for both day and night time.
		for (int j = 0; j < 2*M; ++j){
			bonds[i][j] = 0;
		}	
	}	
	
	unsigned int Seed2 = 123;
	int Nk;	
	float sum;
	
	cout << "Starting simulation loop" << endl;
    for(int i_sim = 0; i_sim < N; ++i_sim){
		for (int i = 0; i < n; ++i){
			final_size[i][i_sim] = 0;
			for (int k = 0; k < M*2; ++k){
				I_this[i][k] = 0;
			}
		}		
		Graph G_current;
		G_current.copy_graph(G);
		cout << "Starting simulation" << i_sim <<  endl;
		for(int i_day = 0; i_day < M; ++i_day){
			if(G_current.locations[40].S > 10){  // Number 40 is Oslo
				G_current.locations[40].I += 10;
				G_current.locations[40].S -= 10;
			}	
			
			
			
			//VACCINATION STEP
			double val = 0; 
			int num; 		
			int day_start = 75; // Day to start vaccination 
			int day_end = 75 + 42; // Day to end vaccination 
			float v_e = 0.7; // Proportion of vaccinated who become immune
			double pv; //Proportion of population in specific location to vaccinate on this day in this location
			int s_v; //Number we were supposed to vaccinate in the location on the day (given enough susceptibles here). 
			int sum_out_links; 
			if (i_day >= day_start && i_day < day_end){
				for (int i = 0; i<n; ++i){ // Vaccinate in these locations 
					sum_out_links = 0;
					int dv = 0; // Counter for number vaccinated
					int s_v = 0;
					pv = 0.05;  
					num = G_current.locations[i].out_links.size();
					val = round(pv*(G_current.locations[i].S+G_current.locations[i].E+G_current.locations[i].I+G_current.locations[i].Ia+G_current.locations[i].R+G_current.locations[i].V + G_current.locations[i].VI + G_current.locations[i].VIa + G_current.locations[i].SVI + G_current.locations[i].VE));
					s_v += val;
					if(G_current.locations[i].S >= val){
						G_current.locations[i].V += round(v_e*val);
						G_current.locations[i].S -= val;
						G_current.locations[i].SVI += val-round(v_e*val);
						dv += val;
					}
					else{
						val = G_current.locations[i].S;
						G_current.locations[i].V += round(v_e*val);
						G_current.locations[i].S -= val;
						G_current.locations[i].SVI += val-round(v_e*val);
						dv += val;
					}		
					sum_out_links += G_current.locations[i].S;
					for (int j = 0; j < num; ++j){
						val = round(pv*(G_current.locations[i].out_links[j]->S + G_current.locations[i].out_links[j]->E + G_current.locations[i].out_links[j]->I + G_current.locations[i].out_links[j]->Ia + G_current.locations[i].out_links[j]->R + G_current.locations[i].out_links[j]->V + G_current.locations[i].out_links[j]->SVI + G_current.locations[i].out_links[j]->VI + G_current.locations[i].out_links[j]->VIa + G_current.locations[i].out_links[j]->VE));
						s_v += val;
						if (G_current.locations[i].out_links[j]->S >= val){
							G_current.locations[i].out_links[j]->V += round(v_e*val);
							G_current.locations[i].out_links[j]->S -= val;
							G_current.locations[i].out_links[j]->SVI += val-round(v_e*val);
							dv += val;
							sum_out_links += G_current.locations[i].out_links[j]->S;
						}
						else{
							val = G_current.locations[i].out_links[j]->S; 
							G_current.locations[i].out_links[j]-> V += round(v_e*val);
							G_current.locations[i].out_links[j]->S -= val;
							G_current.locations[i].out_links[j]->SVI += val - round(v_e*val);
							dv += val;
							sum_out_links += G_current.locations[i].out_links[j]->S;
						}	
					}
					double randomnumber; 
					
					int nv;
					if (sum_out_links > 0 && dv < s_v){
						if (sum_out_links > s_v - dv){
								nv = s_v - dv;
						}
						else{
							nv = sum_out_links;
						}
						for (int m = 0; m < nv; m++){
							double *probs = new double[num+1];
							double *probs_cum = new double[num+1];
							for (int l = 0; l<num; l++){
								probs[l] = G_current.locations[i].out_links[l]->S;
							}
							probs[num] = G_current.locations[i].S;
							int popsum = 0;
							for (int ki = 0; ki < num+1; ki++){
								popsum += probs[ki];
							}
							for (int ki = 0; ki < num+1; ki++){
								probs[ki] /= popsum;
							}					
							cumulative_sum(&probs_cum, &probs, num+1);
							randomnumber = double(rand())/RAND_MAX;
							int index_ = -1;
							for(int h = 0; h < num+1; ++h){
								if(randomnumber < probs_cum[h]){
									index_ = h;
									break;
								}	
							}		 
							if(index_ == num){
								/// Then we did not pick an outlink	
								randomnumber = double(rand())/RAND_MAX;
								if(randomnumber < v_e){
									G_current.locations[i].S -= 1;
									G_current.locations[i].V += 1;
									dv += 1;
								}
								else{
									G_current.locations[i].S -= 1;
									G_current.locations[i].SVI += 1;
									dv += 1;								
								}
							}
							else if(index_ >= 0){
								randomnumber = double(rand())/RAND_MAX;
								if(randomnumber < v_e){
									G_current.locations[i].out_links[index_]->S -= 1;
									G_current.locations[i].out_links[index_]->V += 1;
									dv += 1;							
								}
								else{
									G_current.locations[i].out_links[index_]->S -= 1;
									G_current.locations[i].out_links[index_]->SVI += 1;
									dv += 1;					
								}
							}
							delete[] probs;
							delete[] probs_cum;	
						}
					}					
					
					if (dv != 0 && s_v != dv){
						cout << " Was supposed to vaccinate " << s_v << " vaccinated " << dv <<  " on day " << i_day << " in location " << i << endl;
					}	
				}	
			}		
			// END VACCINATION
			
			
			
			
			
			for (int i = 0; i < n; ++i){
				int de2 = 0; //New symptomatic cases from susceptible 
				int dve2 = 0; // New symptomatic cases from susceptible and vaccinated 
				// Let commuters mix at their work location during day time 
				G_current.locations[i].seir_step_day(beta, a, gamma, de2, dve2);				
				values[i][2*i_day][0] += G_current.locations[i].S;
				values[i][2*i_day][1] += G_current.locations[i].E;
				values[i][2*i_day][2] += G_current.locations[i].I;
				values[i][2*i_day][3] += G_current.locations[i].Ia;
				values[i][2*i_day][4] += G_current.locations[i].R;
				values[i][2*i_day][5] += G_current.locations[i].VI;
				values[i][2*i_day][6] += G_current.locations[i].VIa;
				I_this[i][2*i_day] += G_current.locations[i].I + G_current.locations[i].VI;	
				values[i][2*i_day][7] += de2 + dve2;
				bonds[i_sim][2*i_day] += G_current.locations[i].I + G_current.locations[i].Ia + G_current.locations[i].VI + G_current.locations[i].VIa;
				int num = G_current.locations[i].out_links.size();
				for(int j = 0; j < num; ++j){
					values[i][2*i_day][0] += G_current.locations[i].out_links[j]->S;
					values[i][2*i_day][1] += G_current.locations[i].out_links[j]->E;
					values[i][2*i_day][2] += G_current.locations[i].out_links[j]->I;
					values[i][2*i_day][3] += G_current.locations[i].out_links[j]->Ia;
					values[i][2*i_day][4] += G_current.locations[i].out_links[j]->R;
					values[i][2*i_day][5] += G_current.locations[i].out_links[j]->VI;
					values[i][2*i_day][6] += G_current.locations[i].out_links[j]->VIa;
					I_this[i][2*i_day] += G_current.locations[i].out_links[j] -> I + G_current.locations[i].out_links[j]->VI;
					bonds[i_sim][2*i_day] += G_current.locations[i].out_links[j]->I + G_current.locations[i].out_links[j]->Ia + G_current.locations[i].out_links[j]->VI + G_current.locations[i].out_links[j]->VIa;
				}
			}		
			for (int i = 0; i < n; ++i){	
				int de2 = 0;	
				int dve2 = 0;	
				// Let commuters mix in their home location during night time		
				G_current.locations[i].seir_step_night(beta, a, gamma, de2, dve2);			
				if(i_day == (M-1)){
					final_size[i][i_sim] += G_current.locations[i].R;
				}	
				values[i][2*i_day+1][0] += G_current.locations[i].S;
				values[i][2*i_day+1][1] += G_current.locations[i].E;
				values[i][2*i_day+1][2] += G_current.locations[i].I;
				values[i][2*i_day+1][3] += G_current.locations[i].Ia;
				values[i][2*i_day+1][4] += G_current.locations[i].R;
				values[i][2*i_day+1][5] += G_current.locations[i].VI;
				values[i][2*i_day+1][6] += G_current.locations[i].VIa;
				I_this[i][2*i_day+1] += G_current.locations[i].I + G_current.locations[i].VI;
				values[i][2*i_day+1][7] += de2 + dve2;
				bonds[i_sim][2*i_day+1] += G_current.locations[i].I + G_current.locations[i].Ia + G_current.locations[i].VI + G_current.locations[i].VIa;
				int num = G_current.locations[i].out_links.size();
				for(int j = 0; j < num; ++j){
					if (i_day == (M-1)){
						final_size[i][i_sim] += G_current.locations[i].out_links[j]->R;
					}	
					values[i][2*i_day+1][0] += G_current.locations[i].out_links[j]->S;
					values[i][2*i_day+1][1] += G_current.locations[i].out_links[j]->E;
					values[i][2*i_day+1][2] += G_current.locations[i].out_links[j]->I;
					values[i][2*i_day+1][3] += G_current.locations[i].out_links[j]->Ia;
					values[i][2*i_day+1][4] += G_current.locations[i].out_links[j]->R;
					values[i][2*i_day+1][5] += G_current.locations[i].out_links[j]->VI;
					values[i][2*i_day+1][6] += G_current.locations[i].out_links[j]->VIa;
					I_this[i][2*i_day+1] += G_current.locations[i].out_links[j]->I + G_current.locations[i].VI;
					bonds[i_sim][2*i_day+1] += G_current.locations[i].out_links[j]->I + G_current.locations[i].out_links[j]->Ia + G_current.locations[i].out_links[j]->VI + G_current.locations[i].out_links[j]->VIa;
				}								
			}
		}
		// Initial dates and peak dates. 
		float baseline;
		int pop = 0;
		int pday = 0;
		int pmax = 0;
		int count;
		int startday = 0;
		for(int i = 0; i < n; i++){
			pday = 0;
			pmax = 0;
			startday = 0;
			count = 0;
			pop = G_current.locations[i].S +  G_current.locations[i].E +  G_current.locations[i].I +  G_current.locations[i].Ia +  G_current.locations[i].R + G_current.locations[i].V + G_current.locations[i].SVI+ G_current.locations[i].VI + G_current.locations[i].VIa + G_current.locations[i].VE;
			int num = G.locations[i].out_links.size();
			for (int j = 0; j<num; ++j){
				pop += G_current.locations[i].out_links[j]->S + G_current.locations[i].out_links[j]->E + G_current.locations[i].out_links[j]->I + G_current.locations[i].out_links[j]->Ia + G_current.locations[i].out_links[j]->R;
			}
			if(pop < 100){
				baseline = 1;
			}
			else{
				baseline = 0.01*pop; 
			}		  
			for(int i_day = 0; i_day < M; ++i_day){
				if(I_this[i][2*i_day] > pmax){
					pday = i_day;
					pmax = I_this[i][2*i_day];
				}
				if(I_this[i][2*i_day] > baseline){
					count += 1;
					if (count > 6){
						startday = i_day;
						count = -20000;
					}
				}
			}
			peak_date[i][i_sim] = pday;
			peak_val[i][i_sim] = pmax;
			start_date[i][i_sim] = startday;		
		}				
					
			
		cout << endl << "Done simulating" << endl << endl;
	}
	cout << "Done with all simulations" << endl;
    
    
    ofstream out_data("cpp_res_series.txt");

    
    for(int i=0; i < n; ++i){
		for (int k = 0; k<2*M; ++k){
			out_data << values[i][k][0]*1.0/N << " " << values[i][k][1]*1.0/N << " " << values[i][k][2]*1.0/N << " " << values[i][k][3]*1.0/N << " " << values[i][k][4]*1.0/N << " " << values[i][k][5]*1.0/N << " " << values[i][k][6]*1.0/N  << " "<< values[i][k][7]*1.0/N << endl;

		}
	}	
			
    out_data.close();
    ofstream out_data2("cpp_res_dates.txt");
    for(int i_sim=0; i_sim < N; ++i_sim){
		for (int i = 0; i< n; ++i){
			out_data2 << start_date[i][i_sim] << " " << peak_date[i][i_sim] << " " << peak_val[i][i_sim] << " " <<final_size[i][i_sim] << endl;

		}
	}	
			
    out_data2.close();  
  
    ofstream out_data3("cpp_res_bonds.txt");
    for (int i_sim=0; i_sim < N; ++i_sim){
		for (int i = 0; i < 2*M; ++i){
			out_data3 << bonds[i_sim][i] << " ";
		}
		out_data3 << endl;
	}	             
    
	for(int i = 0; i < n; ++i){
		for(int k = 0; k < 2*M; ++k){
			delete[] values[i][k];	
		}
		delete[] values[i];
	}
	delete[] values;
	for(int i = 0; i < n; ++i){
			delete[] I_this[i];
			delete[] peak_date[i];
			delete[] peak_val[i];
			delete[] start_date[i];
			delete[] final_size[i];
	}
	
	for (int i = 0; i < N; ++i){
		delete[] bonds[i];
	}	
		
	delete[] I_this;
	delete[] peak_date;
	delete[] start_date;
	delete[] peak_val;
	delete[] final_size;
	delete[] bonds;
}

