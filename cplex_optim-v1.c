
/* Bring in the CPLEX function declarations and the C library 
header file stdio.h with the following single include. */

#include <ilcplex/cplex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include declaration for functions at end of program */

//#define NUM_PM 32 // no of physical machines
#define NUM_VM 160 // no of virtual machines
#define NUM_RT 3 // resource types .. IMP -> keep this constant for now
#define MAX_ITER 10
#define FEASIBLE_FLAG 0 // 0 to continue even if a algo fails to give solution AND 1 to run only feasible problems. NOTE : 0 will not run if OPTIMAL LP doesnt give a solution
#define CAPACITY_TYPE 1 // 0 is constant capacity for each case AND 1 is Random Capacityfor every case.
#define RANDOM_TYPE 1 // 1 is for Uniform Requirement AND 2 is for Rand time seed for Requirements. 
#define MACHINE_TYPE 1 // 0 is for Static defined machines and 1 for random Machines. 
// NOTE : CAPACITY_TYPE and RANDOM_TYPE will work for MACHINE_TYPE 1.

struct Machine {
	int cores;
	int memory;
	int storage;
};

static int
	populatebynonzero (CPXENVptr env, CPXLPptr lp, int ***R, int ***C, int NUM_PM);

static void
	free_and_null     (char **ptr),
	usage             (char *progname);

void initializeVariables(int ***R, int ***C, int **RC, int **TC, int **TR, int **minRC, int NUM_PM) {

	//	bool const resourceType = true; // use different capacites for PMs
	int i = 0;
	/*	
	// allocate Bij VMi -> PMj 
	int **B = (int **)malloc(NUM_VM * sizeof(int *));
	for (int i=0; i<NUM_VM; i++)
	B[i] = (int *)malloc(NUM_PM * sizeof(int));

	*/
	//allocate Rik VMi -> RTk
	*R = (int **)malloc(NUM_VM * sizeof(int *));
	for (i=0; i<NUM_VM; i++)
		(*R)[i] = (int *)malloc(NUM_RT * sizeof(int));

	//allocate Cjk PMj -> RTk
	*C = (int **)malloc(NUM_PM * sizeof(int *));
	for (i=0; i<NUM_PM; i++)
		(*C)[i] = (int *)malloc(NUM_RT * sizeof(int));

	/*
	//allocate Ij
	int *I = (int *)malloc(NUM_PM * sizeof(int) );
	*/

	//allocate Resource type Capacity for PM
	*RC = (int *)malloc(NUM_RT * sizeof(int));
	*TC = (int *)malloc(NUM_RT * sizeof(int));
	*TR = (int *)malloc(NUM_RT * sizeof(int));
	*minRC = (int *)malloc(NUM_RT * sizeof(int));

	(*RC)[0] = (*minRC)[0] = 16; // Maximum no of cores for 1 PM
	(*RC)[1] = (*minRC)[1] = 12; // Maximum Ram for 1 PM (Gb)
	(*RC)[2] = (*minRC)[2] = 1024/2; // Maximum HDD space for 1 PM (Gb)

	(*TC)[0] = 6; //minimum no of cores for 1 PM
	(*TC)[1] = 8; //minimum no of RAM for 1 PM
	(*TC)[2] = 100; //minimum no of HDD space for 1 PM

	(*TR)[0] = 0;
	(*TR)[1] = 0;
	(*TR)[2] = 0;

}

void generateRandomCase(int ***R, int ***C, int **RC, int **TC, int **TR, int **minRC, int iter, int randomType, int capacityType, int machineType, int NUM_PM) {

	int tempRA[NUM_RT], k, j, i, tempRTotal = 0;
	int n_PMTypes = 4, n_VMTypes = 7, tempMachine = 0, tempRTotalM[NUM_RT], ctr = 0;
	struct Machine PM[4], VM[7];

	for(k=0; k<NUM_RT; k++)  {
		tempRA[k] = 0;
		tempRTotalM[k] = 0;
	}

	if(  machineType ) {

		//	cout << "tempRA initial " << tempRA[0] << endl;
		if ( capacityType ) {
			srand (time(NULL));
			// initalize Cjk
			for(j=0; j<NUM_PM; j++) {
				for( k=0; k<NUM_RT; k++)  {
					(*C)[j][k] = (*TC)[k] + rand() % ((*RC)[k] - (*TC)[k] + 1);
					//				cout << "c" << j << k << " = " << C[j][k] << endl;

					if( (*C)[j][k] < (*minRC)[k] )
						(*minRC)[k] = (*C)[j][k] - 1;
					(*TR)[k]  += (*C)[j][k];
				}
			}
		}

		srand (time(NULL)+iter);
		// initialize Rik
		for( i=0; i<NUM_VM; i++) {
			for( k=0; k<NUM_RT; k++)  {
				//			cout << maxRC[k] << " divide " << NUM_VM << endl;
				if( randomType == 1) {
					//				float tempr = 1 + rand() % ((*minRC)[k]);
					float tempr =  rand() % (((*TR)[k]/NUM_VM)+1);
					(*R)[i][k] = 1 + (int)tempr % (*TC)[k];
	//				(*R)[i][k] = 1 + (int)tempr % (*minRC)[k];
					//				cout << "r" << i << k << " = " << (*R)[i][k] << " and minRC" << k << " = " << (*minRC)[k] << endl;
					tempRA[k] += (*R)[i][k];
				}
				else {
					do {
						tempRTotal = tempRA[k];
			//			(*R)[i][k] = rand() % (*minRC)[k];
						(*R)[i][k] = rand() % (*TC)[k];
						if( (*R)[i][k] == 0 ) (*R)[i][k] = 1;
						tempRTotal += (*R)[i][k];
					}while( tempRTotal >= (*TR)[k] );
					tempRA[k] += (*R)[i][k];
					//				cout << "tempRA k " << tempRA[k] << endl;
					//				cout << "Req Total for RT " << k << " = " << tempRA[k] << " maxCap " << (*TR)[k] << endl;
				}
				//			cout << "r" << i << k << " = " << (*R)[i][k] << " and minRC" << k << " = " << (*minRC)[k] << endl;
				//			TC[k] -= (int) R[i][k];
			}
		}
	}
	else {

		PM[0].cores = 2;
		PM[0].memory = 32;
		PM[0].storage = 1500;

		PM[1].cores = 4;
		PM[1].memory = 64;
		PM[1].storage = 5000;

		PM[2].cores = 8;
		PM[2].memory = 128;
		PM[2].storage = 6000;

		PM[3].cores = 16;
		PM[3].memory = 256;
		PM[3].storage = 7000;

		VM[0].cores = 1;
		VM[0].memory = 2;
		VM[0].storage = 160;

		VM[1].cores = 2;
		VM[1].memory = 8;
		VM[1].storage = 840;

		VM[2].cores = 4;
		VM[2].memory = 16;
		VM[2].storage = 1680;

		VM[3].cores = 2;
		VM[3].memory = 2;
		VM[3].storage = 340;

		VM[4].cores = 8;
		VM[4].memory = 8;
		VM[4].storage = 1680;

		VM[5].cores = 8;
		VM[5].memory = 32;
		VM[5].storage = 840;

		VM[6].cores = 8;
		VM[6].memory = 64;
		VM[6].storage = 1700;

		srand(time(NULL));

		for( i=0; i<NUM_PM; i++) {

			tempMachine = rand() % (n_PMTypes-1);
			(*C)[i][0] = PM[tempMachine].cores;
			(*C)[i][1] = PM[tempMachine].memory;
			(*C)[i][2] = PM[tempMachine].storage;

			(*TR)[0]  += (*C)[i][0];
			(*TR)[1]  += (*C)[i][1];
			(*TR)[2]  += (*C)[i][2];

			//			printf("Cap for PM %d RT 0 = %d RT 1 = %d RT 2 = %d\n", i, (*C)[i][0], (*C)[i][1], (*C)[i][2]);

		}
		/*
		for( i=0; i<NUM_RT; i++) {
		printf("Total Cap RT %d = %d\n", i, (*TR)[i]);
		}
		*/
		for( i=0; i<NUM_VM; i++) {
			//printf("Temp Total RT 0 = %d RT 1 = %d RT 2 = %d\n", tempRTotalM[0], tempRTotalM[1], tempRTotalM[2]);
			ctr = 0;
			do {
				if( ctr > 50 ) {
//					printf("Too many Iterations\n");	
					break;
				}
				srand(time(NULL)+ctr);
				//printf("Inside 2 loop VMs do while\n");
				tempMachine = rand() % (n_VMTypes-1);

				tempRTotalM[0] = tempRA[0];
				tempRTotalM[1] = tempRA[1];
				tempRTotalM[2] = tempRA[2];

				(*R)[i][0] =  VM[tempMachine].cores;
				(*R)[i][1] =  VM[tempMachine].memory;
				(*R)[i][2] =  VM[tempMachine].storage;

				tempRTotalM[0] += (*R)[i][0];
				tempRTotalM[1] += (*R)[i][1];
				tempRTotalM[2] += (*R)[i][2];

				ctr++;

			}while( tempRTotalM[0] > (*TR)[0] || tempRTotalM[1] > (*TR)[1] || tempRTotalM[2] > (*TR)[2] );

			//printf("Found Req at ctr %d\n", ctr);

			tempRA[0] += (*R)[i][0];
			tempRA[1] += (*R)[i][1];
			tempRA[2] += (*R)[i][2];

			//			printf("Req for VM %d RT 0 = %d RT 1 = %d RT 2 = %d\n", i, (*R)[i][0], (*R)[i][1], (*R)[i][2]);
		}
		//printf("After Assigning Req\n");
	}

	/*
	for( k=0; k<NUM_RT; k++ ) {
	cout << "total req is for RT " << k << " " << tempRA[k] << " Total Cap " << (*TR)[k] <<  endl;
	}
	*/

}


int checkCompatibility(int req[], int cap[]) {

	int tempComp = 1, k;
	for( k=0; k < NUM_RT; k++ ) {
		//		cout << "Req " << req[k] << " Cap " << cap[k] << endl;
		if( req[k] > cap[k] ) {
			tempComp = 0;
			//			cout << "Req " << req[k] << " Cap " << cap[k] << endl;
		}
	}
	return tempComp;
}

int assignReq(int req[], int **remainPMCap) {
	int i;
	for( i=0; i < NUM_RT; i++ ) {
		(*remainPMCap)[i] -= req[i];
	}
	return 1;
}

int generateFirstFit(int ***R, int ***C, int NUM_PM) {

	int **remainPMCap = NULL;
	int usedPM[NUM_PM], i, j, k;
	int num_pmused = 0, compFound = 0;

	for( j=0; j < NUM_PM; j++ ) {
		usedPM[j] = 0;
		//		cout << usedPM[j] << endl;
	}

	remainPMCap = (int **)malloc(NUM_PM * sizeof(int *));
	for ( i=0; i<NUM_PM; i++)
		(remainPMCap)[i] = (int *)malloc(NUM_RT * sizeof(int));

	// fill the capacities of PM
	for( j=0; j < NUM_PM; j++ ) {
		for( k=0; k < NUM_RT; k++ ) {
			remainPMCap[j][k] = (*C)[j][k];
			//			if(j==0)
			//			cout << remainPMCap[j][k] << " = " << (*C)[j][k] << endl;
		}
	}

	for( i=0; i < NUM_VM; i++) {
		compFound = 0;
		for( j=0; j < NUM_PM; j++ ) {
			int tempCompatibility = 0;
			tempCompatibility = checkCompatibility((*R)[i],remainPMCap[j]);	
			//			cout << "compability VM " << i << " with PM " << j << " = " << tempCompatibility << endl;
			if( tempCompatibility ) {
				//				cout << "VM " << i << " Mapped to PM " << j << endl; 
				//				printf("VM %d Mapped to PM %d\n",i,j);
				assignReq((*R)[i], &remainPMCap[j]);
				//				printf("Remaining PM %d cap for RT 0 = %d, 1 = %d, 2 = %d", j, remainPMCap[j][0], remainPMCap[j][1], remainPMCap[j][2]);
				usedPM[j]++;
				compFound = 1;
				break;
			}
			/*
			cout << " For PM " << j << "and VM" << i << endl;
			for( k=0; k < NUM_RT; k++ ) {
			cout << "Not Compatible RT " << k << " Req is " << (*R)[i][k] << " and Cap is " << (*C)[j][k] << endl;
			}
			*/
		}
		if( !compFound ) {
//			printf("couldnt find fit for VM %d\n",i);
			return -1;
		}
		//				cout << "couldnt find fit for VM " << i << endl;
	}

	for( i=0; i<NUM_PM; i++ ) {
		//		cout << "PM used " << usedPM[i] << endl;
		if( usedPM[i] > 0 )
			num_pmused++;
	}
	printf("PM used by First Fit %d\n",num_pmused);
	//	cout << "PM used by First Fit " << num_pmused << endl;
	//	free(usedPM);
	return num_pmused;
}

int generateNextFit(int ***R, int ***C, int NUM_PM) {

	int **remainPMCap = NULL;
	int usedPM[NUM_PM], i, j, k;
	int num_pmused = 0, compFound = 0, lastPMUsed = 0;

	for( j=0; j < NUM_PM; j++ ) {
		usedPM[j] = 0;
		//		cout << usedPM[j] << endl;
	}

	remainPMCap = (int **)malloc(NUM_PM * sizeof(int *));
	for ( i=0; i<NUM_PM; i++)
		(remainPMCap)[i] = (int *)malloc(NUM_RT * sizeof(int));

	// fill the capacities of PM
	for( j=0; j < NUM_PM; j++ ) {
		for( k=0; k < NUM_RT; k++ ) {
			remainPMCap[j][k] = (*C)[j][k];
			//			if(j==0)
			//			cout << remainPMCap[j][k] << " = " << (*C)[j][k] << endl;
		}
	}

	for( i=0; i < NUM_VM; i++) {
		compFound = 0;
		for( j=lastPMUsed; j < NUM_PM; j++ ) {
			int tempCompatibility = 0;
			tempCompatibility = checkCompatibility((*R)[i],remainPMCap[j]);	
			//			cout << "compability VM " << i << " with PM " << j << " = " << tempCompatibility << endl;
			if( tempCompatibility ) {
				//				cout << "VM " << i << " Mapped to PM " << j << endl; 
				//				printf("VM %d Mapped to PM %d\n",i,j);
				assignReq((*R)[i], &remainPMCap[j]);
				lastPMUsed = j;
				//				printf("Remaining PM %d cap for RT 0 = %d, 1 = %d, 2 = %d", j, remainPMCap[j][0], remainPMCap[j][1], remainPMCap[j][2]);
				usedPM[j]++;
				compFound = 1;
				break;
			}
			if( lastPMUsed == NUM_PM-1 ) lastPMUsed = 0;
			/*
			cout << " For PM " << j << "and VM" << i << endl;
			for( k=0; k < NUM_RT; k++ ) {
			cout << "Not Compatible RT " << k << " Req is " << (*R)[i][k] << " and Cap is " << (*C)[j][k] << endl;
			}
			*/
		}
		if( !compFound ) {
//			printf("couldnt find fit for VM %d\n",i);
			return -1;
		}
		//				cout << "couldnt find fit for VM " << i << endl;
	}

	for( i=0; i<NUM_PM; i++ ) {
		//		cout << "PM used " << usedPM[i] << endl;
		if( usedPM[i] > 0 )
			num_pmused++;
	}
	printf("PM used by Next Fit %d\n",num_pmused);
	//	cout << "PM used by First Fit " << num_pmused << endl;
	//	free(usedPM);
	return num_pmused;
}

double getRemainPercentage(int req[], int cap[], double **percentage) {

	double avg_remain = 0.0;
	int i = 0;
	for( i=0; i < NUM_RT; i++ ) {
		//		cout << "Req for RT " << i << " = " << req[i] << " and cap " << cap[i] << endl;
		(*percentage)[i] = ((double)cap[i] - (double)req[i]) / (double)cap[i]*100;
		//		cout << "Percentage " << (*percentage)[i] << " value " << (cap[i] - req[i]) / cap[i]*100 << endl;
		avg_remain += (*percentage)[i];
	}
	return avg_remain;
}

int generateBestFit(int ***R, int ***C, int **RC, int **TC, int **TR, int **minRC, int iter, int NUM_PM) {

	int **remainPMCap = NULL;
	int usedPM[NUM_PM], i, j, k, bestPM = NULL, compatiblePM[NUM_PM];
	double **percentRemain, avgRemain[NUM_PM];
	double maxAvg = 0, tempMaxAvg = 0;
	int num_pmused = 0, compFound = 0;

	for( j=0; j < NUM_RT; j++ ) {
		maxAvg += (*TR)[j];
	}
	//	cout << "max avg = " << maxAvg << endl;

	for( j=0; j < NUM_PM; j++ ) {
		usedPM[j] = 0;
		avgRemain[j] = 0;
		compatiblePM[j] = 0;
		//		cout << usedPM[j] << endl;
	}

	remainPMCap = (int **)malloc(NUM_PM * sizeof(int *));
	for ( i=0; i<NUM_PM; i++)
		remainPMCap[i] = (int *)malloc(NUM_RT * sizeof(int));

	percentRemain = (double **)malloc(NUM_PM * sizeof(double *));
	for ( i=0; i<NUM_PM; i++)
		percentRemain[i] = (double *)malloc(NUM_RT * sizeof(double));

	// fill the capacities of PM
	for( j=0; j < NUM_PM; j++ ) {
		for( k=0; k < NUM_RT; k++ ) {
			remainPMCap[j][k] = (*C)[j][k];
			//			if(j==0)
			//			cout << remainPMCap[j][k] << " = " << (*C)[j][k] << endl;
		}
	}

	for( i=0; i < NUM_VM; i++) {
		compFound = 0;
		for( j=0; j < NUM_PM; j++ ) {
			compatiblePM[j] = 0;
			int tempCompatibility = 0;
			tempCompatibility = checkCompatibility((*R)[i],remainPMCap[j]);	
			//			cout << "compability VM " << i << " with PM " << j << " = " << tempCompatibility << endl;
			if( tempCompatibility ) {
				compatiblePM[j] = 1;
				avgRemain[j] = getRemainPercentage((*R)[i],remainPMCap[j],&percentRemain[j]);
				//				cout << "Average Remaining for PM " << j << " AND VM " << i << " = " << avgRemain[j] << endl;
			}
		}
		bestPM = -1;
		tempMaxAvg = maxAvg;
		for( j=0; j < NUM_PM; j++ ) {
			if( compatiblePM[j] ) {
				compFound = 1;
				//				cout << "comparing for PM " << j << "Avg Remain " << avgRemain[j] << " MaxAvg " << tempMaxAvg << endl;
				if( avgRemain[j] < tempMaxAvg ) {
					bestPM = j;
					tempMaxAvg = avgRemain[j];
				}
			}
			/*
			cout << " For PM " << j << "and VM" << i << endl;
			for( k=0; k < NUM_RT; k++ ) {
			cout << "Compatible RT " << k << " Req is " << (*R)[i][k] << " and Cap is " << (*C)[j][k] << endl;
			}
			*/
		}
		if( !compFound || bestPM == -1 ) {
//			printf("couldnt find fit for VM %d\n",i);
			return -1;
			//			cout << "coudnt find best PM for VM " << i << endl;
			//			return 1;
		}
		else {
			//			cout << "VM " << i << " Mapped to PM " << bestPM << endl;
			//			printf("VM %d Mapped to PM %d",i,bestPM);
			assignReq((*R)[i], &remainPMCap[bestPM]);
			usedPM[bestPM]++;
		}
	}

	for( i=0; i<NUM_PM; i++ ) {
		//		cout << "PM used " << usedPM[i] << endl;
		if( usedPM[i] > 0 )
			num_pmused++;
	}
	printf("PM used by Best Fit %d\n",num_pmused);
	//	cout << "PM used by Best Fit " << num_pmused << endl;
	//	free(usedPM);
	return num_pmused;

}

int generateWorstFit(int ***R, int ***C, int **RC, int **TC, int **TR, int **minRC, int iter, int NUM_PM) {

	int **remainPMCap = NULL;
	int usedPM[NUM_PM], i, j, k, bestPM = NULL, compatiblePM[NUM_PM];
	double **percentRemain, avgRemain[NUM_PM];
	double minAvg = 0, tempMinAvg = 0;
	int num_pmused = 0, compFound;

	minAvg = 0;

	//	cout << "max avg = " << maxAvg << endl;

	for( j=0; j < NUM_PM; j++ ) {
		usedPM[j] = 0;
		avgRemain[j] = 0;
		compatiblePM[j] = 0;
		//		cout << usedPM[j] << endl;
	}

	remainPMCap = (int **)malloc(NUM_PM * sizeof(int *));
	for ( i=0; i<NUM_PM; i++)
		remainPMCap[i] = (int *)malloc(NUM_RT * sizeof(int));

	percentRemain = (double **)malloc(NUM_PM * sizeof(double *));
	for ( i=0; i<NUM_PM; i++)
		percentRemain[i] = (double *)malloc(NUM_RT * sizeof(double));

	// fill the capacities of PM
	for( j=0; j < NUM_PM; j++ ) {
		for( k=0; k < NUM_RT; k++ ) {
			remainPMCap[j][k] = (*C)[j][k];
			//			if(j==0)
			//			cout << remainPMCap[j][k] << " = " << (*C)[j][k] << endl;
		}
	}

	for( i=0; i < NUM_VM; i++) {
		compFound = 0;
		for( j=0; j < NUM_PM; j++ ) {
			compatiblePM[j] = 0;
			int tempCompatibility = 0;
			tempCompatibility = checkCompatibility((*R)[i],remainPMCap[j]);	
			//			cout << "compability VM " << i << " with PM " << j << " = " << tempCompatibility << endl;
			if( tempCompatibility ) {
				compatiblePM[j] = 1;
				avgRemain[j] = getRemainPercentage((*R)[i],remainPMCap[j],&percentRemain[j]);
				//				cout << "Average Remaining for PM " << j << " AND VM " << i << " = " << avgRemain[j] << endl;
			}
		}
		bestPM = -1;
		tempMinAvg = minAvg;
		for( j=0; j < NUM_PM; j++ ) {	
			if( compatiblePM[j] ) {
				compFound = 1;
				//				cout << "comparing for PM " << j << "Avg Remain " << avgRemain[j] << " MaxAvg " << tempMaxAvg << endl;
				if( avgRemain[j] > tempMinAvg ) {
					bestPM = j;
					tempMinAvg = avgRemain[j];
				}
			}
			/*
			cout << " For PM " << j << "and VM" << i << endl;
			for( k=0; k < NUM_RT; k++ ) {
			cout << "Compatible RT " << k << " Req is " << (*R)[i][k] << " and Cap is " << (*C)[j][k] << endl;
			}
			*/
		}
		if( !compFound || bestPM == -1) {
//			printf("couldnt find fit for VM %d\n",i);
			return -1;
			//			cout << "coudnt find best PM for VM " << i << endl;
			//			return 1;
		}
		else {
			//			cout << "VM " << i << " Mapped to PM " << bestPM << endl;
			assignReq((*R)[i], &remainPMCap[bestPM]);
			usedPM[bestPM]++;
		}
	}

	for( i=0; i<NUM_PM; i++ ) {
		//		cout << "PM used " << usedPM[i] << endl;
		if( usedPM[i] > 0 )
			num_pmused++;
	}
	printf("PM used by Worst Fit %d\n",num_pmused);
	//	cout << "PM used by Worst Fit " << num_pmused << endl;
	//	free(usedPM);
	return num_pmused;

}

int generateRandomFit(int ***R, int ***C, int **RC, int **TC, int **TR, int **minRC, int iter, int NUM_PM) {

	int **remainPMCap = NULL;
	int usedPM[NUM_PM], i, j, k, bestPM = -1, compatiblePM[NUM_PM];
	int num_pmused = 0, randPM, compFound = 0, compPMListlen = 0, ctr = 0;

	//	cout << "max avg = " << maxAvg << endl;
	for( j=0; j < NUM_PM; j++ ) {
		usedPM[j] = 0;
		compatiblePM[j] = -1;
		//		cout << usedPM[j] << endl;
	}

	remainPMCap = (int **)malloc(NUM_PM * sizeof(int *));
	for ( i=0; i<NUM_VM; i++)
		remainPMCap[i] = (int *)malloc(NUM_RT * sizeof(int));

	// fill the capacities of PM
	for( j=0; j < NUM_PM; j++ ) {
		//		printf("hello1 %d\n", j);
		for( k=0; k < NUM_RT; k++ ) {
			remainPMCap[j][k] = (*C)[j][k];
			//			if(j==0)
			//			cout << remainPMCap[j][k] << " = " << (*C)[j][k] << endl;
		}
	}

	for( i=0; i < NUM_VM; i++) {
		compFound = 0;
		//		compPMListlen = 0;
		ctr = 0;
		//		*compPMList = NULL;
		for( j=0; j < NUM_PM; j++ ) {
			compatiblePM[j] = -1;
			int tempCompatibility = 0;
			tempCompatibility = checkCompatibility((*R)[i],remainPMCap[j]);	
			//			cout << "compability VM " << i << " with PM " << j << " = " << tempCompatibility << endl;
			if( tempCompatibility ) {
				//				compPMListlen++;
				//				compPMList = (int *)realloc(compPMList, compPMListlen * sizeof(int));
				//				compPMList[compPMListlen-1] = j;
				compatiblePM[ctr] = j;
				compFound = 1;
				//				avgRemain[j] = getRemainPercentage((*R)[i],remainPMCap[j],&percentRemain[j]);
				//				cout << "Average Remaining for PM " << j << " AND VM " << i << " = " << avgRemain[j] << endl;
				ctr++;
			}
		}
		if( !compFound ) {
//			printf("couldnt find fit for VM %d\n",i);
			return -1;
			//			cout << "coudnt find best PM for VM " << i << endl;
			//			return 1;
		}
		else {
			/*
			for( j=0; j<NUM_PM; j++ ) {
			printf("All Compatible PMs at ctr %d = %d\n", j, compatiblePM[j]);
			}
			*/
			srand (time(NULL));
			//			if( compPMListlen == 1 ) randPM = compPMList[0];
			//			else randPM = rand() % (compPMListlen-1);
			if( ctr == 1 ) randPM = 0;
			else randPM = rand() % (ctr-1);
			/*
			do {
			srand (time(NULL));
			randPM = rand() % (NUM_PM-1);
			}while( !compatiblePM[randPM] );
			*/
			//			cout << "VM " << i << " Mapped to PM " << bestPM << endl;
			assignReq((*R)[i], &remainPMCap[compatiblePM[randPM]]);
			usedPM[compatiblePM[randPM]]++;
		}	
		/*
		cout << " For PM " << j << "and VM" << i << endl;
		for( k=0; k < NUM_RT; k++ ) {
		cout << "Compatible RT " << k << " Req is " << (*R)[i][k] << " and Cap is " << (*C)[j][k] << endl;
		}
		*/
	}

	for( i=0; i<NUM_PM; i++ ) {
		//		cout << "PM used " << usedPM[i] << endl;
		if( usedPM[i] > 0 )
			num_pmused++;
	}
	printf("PM used by Random Fit %d\n", num_pmused);
	//	cout << "PM used by Random Fit " << num_pmused << endl;
	//	free(usedPM);
	return num_pmused;

}

int
	main (int argc, char **argv)
{
	/* Declare and allocate space for the variables and arrays where we
	will store the optimization results including the status, objective
	value, variable values, dual values, row slacks and variable
	reduced costs. */

	int      solstat;
	double   objval;
	/*   double   *x = NULL;
	double   *pi = NULL;
	double   *slack = NULL;
	double   *dj = NULL;
	*/

	CPXENVptr     env = NULL;
	CPXLPptr      lp = NULL;
	int           status = 0;
	int           i, j;
	int           cur_numrows, cur_numcols;


	int iter = 0;
	int **R = NULL;
	int **C = NULL;
	int *RC = NULL;
	int *TC = NULL;
	int *TR = NULL;
	int *minRC = NULL;
	int NUM_PM = 0, n;

	int avg_Result[6][MAX_ITER];
	float tempavg[6];

	FILE *fp, *fp1;
	int nPMUsed_out = 0;

	fp = fopen("output.csv", "w+");
	if( fp != NULL ) {
		//		fprintf(fp, "This is testing for fprintf...\n");
		//		fputs("This is testing for fputs...\n", fp);
	}
	else {
		printf("Couldn't open file to write ouput");
	}

	for( n=0; n<3; n++ ) {

		if( n == 0 ) NUM_PM = NUM_VM/2;
		else if( n == 1 && NUM_VM == 32 ) NUM_PM = 24;
		else if( n == 1 && NUM_VM == 64 ) NUM_PM = 48;
		else if( n == 1 && NUM_VM == 128 ) NUM_PM = 96;
		else NUM_PM = NUM_VM;

//printf("Number of PM for n %d is %d\n", n, NUM_PM);

		initializeVariables(&R, &C, &RC, &TC, &TR, &minRC, NUM_PM);

		//	initializeVariables(&R, &C, &RC, &TC, &TR, &minRC);
		if( CAPACITY_TYPE == 0 )
			generateRandomCase(&R, &C, &RC, &TC, &TR, &minRC, 1, 1, 1, 1, NUM_PM);

		/* Check the command line arguments */
		/*
		if (( argc != 2 )                                         ||
		( argv[1][0] != '-' )                                 ||
		( strchr ("rcn", argv[1][1]) == NULL )  ) {
		usage (argv[0]);
		goto TERMINATE;
		}
		*/

		for( iter=1; iter<=MAX_ITER; iter++) {
			//printf("ITER %d\n", iter);
			if( fp == NULL ) break;

			/* Initialize the CPLEX environment */

			env = CPXopenCPLEX (&status);

			/* If an error occurs, the status value indicates the reason for
			failure.  A call to CPXgeterrorstring will produce the text of
			the error message.  Note that CPXopenCPLEX produces no output,
			so the only way to see the cause of the error is to use
			CPXgeterrorstring.  For other CPLEX routines, the errors will
			be seen if the CPXPARAM_ScreenOutput indicator is set to CPX_ON.  */

			if ( env == NULL ) {
				char  errmsg[CPXMESSAGEBUFSIZE];
				fprintf (stderr, "Could not open CPLEX environment.\n");
				CPXgeterrorstring (env, status, errmsg);
				fprintf (stderr, "%s", errmsg);
				goto TERMINATE;
			}

			/* Turn on output to the screen */

			status = CPXsetintparam (env, CPXPARAM_ScreenOutput, CPX_OFF);
			if ( status ) { 
				fprintf (stderr, 
					"Failure to turn on screen indicator, error %d.\n", status);
				goto TERMINATE;
			}

			/* Turn on data checking */

			status = CPXsetintparam (env, CPXPARAM_Read_DataCheck, CPX_ON);
			if ( status ) {
				fprintf (stderr, 
					"Failure to turn on data checking, error %d.\n", status);
				goto TERMINATE;
			}
			//printf("Before Random Case Gen\n");
			if( CAPACITY_TYPE == 0)
				generateRandomCase(&R, &C, &RC, &TC, &TR, &minRC, iter, 1, 0, MACHINE_TYPE, NUM_PM);
			else 
				generateRandomCase(&R, &C, &RC, &TC, &TR, &minRC, iter, RANDOM_TYPE, 1, MACHINE_TYPE, NUM_PM);
			//printf("After Random Case Gen\n");
			fprintf(fp, "%d ", iter);
			//printf("Before Heuristic Algo\n");
			nPMUsed_out = generateFirstFit(&R, &C, NUM_PM);
			avg_Result[0][iter-1] = nPMUsed_out;
			fprintf(fp, ", %d ", nPMUsed_out);
			if( nPMUsed_out == -1 && FEASIBLE_FLAG == 1 ) {
				iter--;
				continue;
			}

			nPMUsed_out = generateNextFit(&R, &C, NUM_PM);
			fprintf(fp, ", %d ", nPMUsed_out);
			avg_Result[1][iter-1] = nPMUsed_out;
			if( nPMUsed_out == -1 && FEASIBLE_FLAG == 1 ) {
				iter--;
				continue;
			}

			nPMUsed_out = generateBestFit(&R, &C, &RC, &TC, &TR, &minRC, iter, NUM_PM);
			fprintf(fp, ", %d ", nPMUsed_out);
			avg_Result[2][iter-1] = nPMUsed_out;
			if( nPMUsed_out == -1 && FEASIBLE_FLAG == 1 ) {
				iter--;
				continue;
			}

			nPMUsed_out = generateWorstFit(&R, &C, &RC, &TC, &TR, &minRC, iter, NUM_PM);
			fprintf(fp, ", %d ", nPMUsed_out);
			avg_Result[3][iter-1] = nPMUsed_out;
			if( nPMUsed_out == -1 && FEASIBLE_FLAG == 1 ) {
				iter--;
				continue;
			}

			nPMUsed_out = generateRandomFit(&R, &C, &RC, &TC, &TR, &minRC, iter, NUM_PM);
			fprintf(fp, ", %d ", nPMUsed_out);
			avg_Result[4][iter-1] = nPMUsed_out;
			if( nPMUsed_out == -1 && FEASIBLE_FLAG == 1 ) {
				iter--;
				continue;
			}

			//printf("After Heuristic Algo\n");
			/* Create the problem. */

			lp = CPXcreateprob (env, &status, "lpex1");

			/* A returned pointer of NULL may mean that not enough memory
			was available or there was some other problem.  In the case of 
			failure, an error message will have been written to the error 
			channel from inside CPLEX.  In this example, the setting of
			the parameter CPXPARAM_ScreenOutput causes the error message to
			appear on stdout.  */

			if ( lp == NULL ) {
				fprintf (stderr, "Failed to create LP.\n");
				goto TERMINATE;
			}

			/* Now populate the problem with the data.  For building large
			problems, consider setting the row, column and nonzero growth
			parameters before performing this task. */
			/*
			switch (argv[1][1]) {
			case 'r':
			status = populatebyrow (env, lp);
			break;
			case 'c':
			status = populatebycolumn (env, lp);
			break;
			case 'n':
			status = populatebynonzero (env, lp);
			break;
			}
			*/
			//printf("before Populating\n");
			status = populatebynonzero (env, lp, &R, &C, NUM_PM);
			//printf("After Populating\n");
			if ( status ) {
				fprintf (stderr, "Failed to populate problem.\n");
				goto TERMINATE;
			}

			status = CPXwriteprob (env, lp, "lpex1.lp", NULL);
			if ( status ) {
				fprintf (stderr, "Failed to write LP to disk.\n");
				goto TERMINATE;
			}

			/* Optimize the problem and obtain solution. */

			//  status = CPXlpopt (env, lp);
			status = CPXmipopt (env, lp);
			if ( status ) {
				fprintf (stderr, "Failed to optimize LP.\n");
				//	  fprintf(fp, ", -1 \n");
				iter--;
				continue;
				goto TERMINATE;
			}

			solstat = CPXgetstat (env, lp);
			printf ("Solution status %d.\n", solstat);

			status  = CPXgetobjval (env, lp, &objval);

			if ( status ) {
				fprintf (stderr,"Failed to obtain objective value.\n");
				//	  fprintf(fp, "\n");
				fprintf(fp, ", -1 \n");
				iter--;
				continue;
				//	  continue;
				goto TERMINATE;
			}

			//objective value
			printf ("iter = %d sol = %.10g\n",iter, objval);
			fprintf(fp, ", %.10g \n", objval);
			avg_Result[5][iter-1] = objval;


			/* The size of the problem should be obtained by asking CPLEX what
			the actual size is, rather than using sizes from when the problem
			was built.  cur_numrows and cur_numcols store the current number 
			of rows and columns, respectively.  */
			/*
			cur_numrows = CPXgetnumrows (env, lp);
			cur_numcols = CPXgetnumcols (env, lp);

			x = (double *) malloc (cur_numcols * sizeof(double));
			slack = (double *) malloc (cur_numrows * sizeof(double));
			dj = (double *) malloc (cur_numcols * sizeof(double));
			pi = (double *) malloc (cur_numrows * sizeof(double));

			if ( x     == NULL ||
			slack == NULL ||
			dj    == NULL ||
			pi    == NULL   ) {
			status = CPXERR_NO_MEMORY;
			fprintf (stderr, "Could not allocate memory for solution.\n");
			goto TERMINATE;
			}

			status = CPXsolution (env, lp, &solstat, &objval, x, pi, slack, dj);
			if ( status ) {
			fprintf (stderr, "Failed to obtain solution.\n");
			goto TERMINATE;
			}
			*/
			/* Write the output to the screen. */
			/*
			printf ("\nSolution status = %d\n", solstat);
			printf ("Solution value  = %f\n\n", objval);

			for (i = 0; i < cur_numrows; i++) {
			printf ("Row %d:  Slack = %10f  Pi = %10f\n", i, slack[i], pi[i]);
			}

			for (j = 0; j < cur_numcols; j++) {
			printf ("Column %d:  Value = %10f  Reduced cost = %10f\n",
			j, x[j], dj[j]);
			}
			*/
			/* Finally, write a copy of the problem to a file. */



		}
		
		if( n == 0 && NUM_VM == 32 ) fp1 = fopen("output_hist_1.csv", "w+");
		if( n == 0 && NUM_VM == 64 ) fp1 = fopen("output_hist_2.csv", "w+");
		if( n == 0 && NUM_VM == 128 ) fp1 = fopen("output_hist_3.csv", "w+");
//		else fp1 = fopen("output_hist.csv", "a+");

		if( fp1 != NULL ) {
		//		fprintf(fp, "This is testing for fprintf...\n");
		//		fputs("This is testing for fputs...\n", fp);
			fprintf(fp1, "%d", n+1);
			for( j=0; j<6; j++ ) {
				tempavg[j] = 0;
				for( i=0; i<MAX_ITER; i++) {
					tempavg[j] += avg_Result[j][i];
				}
				tempavg[j] = tempavg[j]/(float)MAX_ITER;
				fprintf(fp1, ",%f", tempavg[j]);
			}
			fprintf(fp1, "\n");
		}
		else {
			printf("Couldn't open file to write output");
		}
	}

TERMINATE:

	printf("In terminate\n");
	/* Free up the solution */
	/*
	free_and_null ((char **) &x);
	free_and_null ((char **) &slack);
	free_and_null ((char **) &dj);
	free_and_null ((char **) &pi);
	*/
	free(R);
	free(C);
	free(TR);
	free(TC);
	free(RC);
	free(minRC);

	if( fp != NULL ) fclose(fp);
	if( fp1 != NULL ) fclose(fp1);

	/* Free up the problem as allocated by CPXcreateprob, if necessary */

	if ( lp != NULL ) {
		status = CPXfreeprob (env, &lp);
		if ( status ) {
			fprintf (stderr, "CPXfreeprob failed, error code %d.\n", status);
		}
	}

	/* Free up the CPLEX environment, if necessary */

	if ( env != NULL ) {
		status = CPXcloseCPLEX (&env);

		/* Note that CPXcloseCPLEX produces no output,
		so the only way to see the cause of the error is to use
		CPXgeterrorstring.  For other CPLEX routines, the errors will
		be seen if the CPXPARAM_ScreenOutput indicator is set to CPX_ON. */

		if ( status ) {
			char  errmsg[CPXMESSAGEBUFSIZE];
			fprintf (stderr, "Could not close CPLEX environment.\n");
			CPXgeterrorstring (env, status, errmsg);
			fprintf (stderr, "%s", errmsg);
		}
	}

	return (status);

}  /* END main */


/* This simple routine frees up the pointer *ptr, and sets *ptr to NULL */

static void
	free_and_null (char **ptr)
{
	if ( *ptr != NULL ) {
		free (*ptr);
		*ptr = NULL;
	}
} /* END free_and_null */  



static void
	usage (char *progname)
{
	fprintf (stderr,"Usage: %s -X\n", progname);
	fprintf (stderr,"   where X is one of the following options: \n");
	fprintf (stderr,"      r          generate problem by row\n");
	fprintf (stderr,"      c          generate problem by column\n");
	fprintf (stderr,"      n          generate problem by nonzero\n");
	fprintf (stderr," Exiting...\n");
} /* END usage */

/* To populate by nonzero, we first create the rows, then create the
columns, and then change the nonzeros of the matrix 1 at a time.  */

static int
	populatebynonzero (CPXENVptr env, CPXLPptr lp, int ***R, int ***C, int NUM_PM)
{
	const int NUMCOLS = NUM_VM * NUM_PM; // variables without the indicator varaibles
	const int NUMROWS = (int)NUM_VM + NUM_PM * NUM_RT; // without the indicator rows
	const int NUMNZ = (int)(NUM_VM * NUM_PM * NUM_RT) + (NUM_VM * NUM_PM); // nonzeros without the indicator ones.
	int      status    = 0;
	double   obj[NUMCOLS + (int)NUM_VM];
	double   lb[NUMCOLS + (int)NUM_VM];
	double   ub[NUMCOLS + (int)NUM_VM];
	char     *colname[NUMCOLS + (int)NUM_VM];
	char     coltype[NUMCOLS + (int)NUM_VM];
	double   rhs[NUMROWS];
	char     sense[NUMROWS];
	char     *rowname[NUMROWS];
	int      rowlist[NUMNZ];
	int      collist[NUMNZ];
	double   vallist[NUMNZ];
	int		i, j, k;
	int		tempRowConst = 0;
	double	linval[NUM_VM]; // COefficients
	int		linind[NUM_VM]; // Variables
	char		*indname[NUM_PM*2];

	//printf("Populating\n");

	//Initialize Nmae arrays
	for( i=0; i<NUMROWS; i++ ) { 
		rowname[i] = malloc(100);
	}
	for( i=0; i<NUMCOLS+NUM_VM; i++ ) { 
		colname[i] = malloc(100);
	}
	for( i=0; i<NUM_PM*2; i++ ) { 
		indname[i] = malloc(100);
	}

	status = CPXchgobjsen (env, lp, CPX_MIN);  /* Problem is minimization */
	if ( status )  goto TERMINATE;
	//printf("After CPX Obj Sense %d\n", sizeof(rowname)/sizeof(rowname[0]));
	/* Now create the new rows.  First, populate the arrays. */
	//rowname[sizeof(rowname)/sizeof(rowname[0])-1] = malloc(100);
	//printf("last element %s", rowname[sizeof(rowname)/sizeof(rowname[0])-1]);
	// Binary Constraints Bij = 1
	for( i=0; i<NUM_VM; i++ ) { 
		//printf("temprowconst in loop %d\n", tempRowConst);
		rowname[tempRowConst] = malloc(100);
		sprintf(rowname[tempRowConst], "c1_%d", i+1);
		//printf("temprowconst in loop %d\n", tempRowConst);
		//		rowname[tempRowConst][0] = "c1";
		//		rowname[tempRowConst][1] = (char)i+1;
		//		itoa(i+1, rowname[tempRowConst][1], 10);
		sense[tempRowConst]   = 'E';
		//printf("temprowconst in loop %d\n", tempRowConst);
		rhs[tempRowConst]     = 1.0;
		//printf("temprowconst in loop %d\n", tempRowConst);
		tempRowConst++;
	}
	//   tempRowConst--;
	//printf("temprowconst after first loop %d\n", tempRowConst);
	// Usage Constraints Ujk < Cjk
	for (j = 0; j < NUM_PM; j++) {
		for (k = 0; k < NUM_RT; k++) {
			//printf("array index %d %d\n", tempRowConst, tempRowConst-NUM_VM+2);
			rowname[tempRowConst] = malloc(100);
			sprintf(rowname[tempRowConst], "c2_%d", tempRowConst-NUM_VM);
			//				rowname[tempRowConst][0] = "c2";
			//				rowname[tempRowConst][1] = (char)k*NUM_RT+j+1;
			//				itoa(k*NUM_RT+j+1, rowname[tempRowConst][1], 10);
			sense[tempRowConst]   = 'L';
			rhs[tempRowConst]     = (*C)[j][k]; // Cjk
			tempRowConst++;
		}
	}
	//printf("temprowconst after 2nd loop %d\n", tempRowConst);

	status = CPXnewrows (env, lp, NUMROWS, rhs, sense, NULL, rowname);
	if ( status )   goto TERMINATE;

	//printf("After CPX new Rows\n");
	/* Now add the new columns.  First, populate the arrays. */

	for (j = 0; j < NUMCOLS + NUM_PM; j++) {
		if( j < NUMCOLS ) {
			obj[j] = 0;
			lb[j] = 0;
			ub[j] = 1;
			sprintf(colname[j], "B%d_%d", (j / NUM_PM)+1, (j % NUM_PM)+1);
			//			colname[j][0] = "B";
			//			colname[j][1] = (char)((j+1) / NUM_VM);
			//			colname[j][2] = (char)((j+1) % NUM_VM);
			//			itoa((j+1) / NUM_VM, colname[j][1], 10);
			//			itoa((j+1) % NUM_VM, colname[j][2], 10);
			coltype[j] = 'B'; // all binary variables Bij
		} 
		else {
			obj[j] = 1;
			lb[j] = 0;
			ub[j] = 1;
			sprintf(colname[j], "I%d", j-NUMCOLS+1);
			//			colname[j][0] = "I";
			//			colname[j][1] = (char)j-NUMCOLS+1;
			//			itoa(j-NUMCOLS+1, colname[j][1], 10);
			coltype[j] = 'B'; // all binary variables Ij
		}
	}

	status = CPXnewcols (env, lp, NUMCOLS + NUM_PM, obj, lb, ub, coltype, colname);
	if ( status )  goto TERMINATE;
	//printf("After CPX new Cols\n");
	/* Now create the list of coefficients */

	int tempRowIndex = 0;
	int tempArrayIndex = 0;
	// Binary Constraints Bij = 1
	for( i=0; i<NUM_VM; i++ ) {   
		for(j=0; j<NUM_PM; j++ ) {
			rowlist[tempArrayIndex] = tempRowIndex;
			//printf("adding col %d\n",  tempArrayIndex);
			collist[tempArrayIndex] = tempArrayIndex;   
			vallist[tempArrayIndex] = 1;	
			tempArrayIndex++;
		}
		tempRowIndex++;
	}
	//   tempArrayIndex--;
	//printf("temparrayindex after BIJ=1 %d %d\n",  tempArrayIndex, tempRowIndex);
	// Usage Constraints Ujk < Cjk
	for (j = 0; j < NUM_PM; j++) {
		for (k = 0; k < NUM_RT; k++) {
			for (i = 0; i < NUM_VM; i++) {
				rowlist[tempArrayIndex] = tempRowIndex;
				//printf("adding col %d = %d\n", tempArrayIndex, i*NUM_PM+j);
				collist[tempArrayIndex] = i*NUM_PM+j;   
				vallist[tempArrayIndex] = (*R)[i][k];	 // multiply Rik
				tempArrayIndex++;
			}
			tempRowIndex++;
		}
	}

	status = CPXchgcoeflist (env, lp, NUMNZ, rowlist, collist, vallist);
	if ( status )  goto TERMINATE;
	//printf("After Coeef added\n");

	tempArrayIndex = 0;
	tempRowIndex = 0;
	// Add Indicator Constraints
	for(i = 0; i < NUM_PM; i++) {
		sprintf(indname[tempRowIndex],"c3_%d",tempRowIndex);
		sprintf(indname[tempRowIndex+1],"c3_%d",tempRowIndex+1);
		//		indname[i][0] = "c3";
		//		indname[i][1] = (char)i+1;
		//		itoa(i+1, indname[i][1], 10);

		for(j = 0; j < NUM_VM; j++) {
			//printf("Linear value %d = %d ", j, j*NUM_PM+i);
			linind[j] = j*NUM_PM+i;
			linval[j] = 1;
			tempArrayIndex++;
		}

		status = CPXaddindconstr (env, lp, NUMCOLS+i, 1, NUM_VM, 
			0.0, 'E', linind, linval, indname[tempRowIndex]);
		if ( status ) {
			fprintf (stderr, "Failed to add indicator constraint.");
			goto TERMINATE;
		}
		tempRowIndex++;

		status = CPXaddindconstr (env, lp, NUMCOLS+i, 0, NUM_VM, 
			1.0, 'G', linind, linval, indname[tempRowIndex]);
		if ( status ) {
			fprintf (stderr, "Failed to add indicator constraint.");
			goto TERMINATE;
		}
		tempRowIndex++;
	}
	//printf("After Preparing Problem\n");

TERMINATE:
	printf("In populating terminate\n");
	return (status);

}  /* END populatebynonzero */

