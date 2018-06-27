/*** Allard Group jun.allard@uci.edu                    ***/

void metropolisJoint();
void stationarity();
void appendBins();
void rotate(double *tIn, double *e1In, double *e2In, double *tOut, double *e1Out, double *e2Out, double phiHere, double thetaHere,double psiHere);

void metropolisJoint()
{

    /********* INITIALIZE ISITES AND BSITES AND BASIC SITES *******************/

    getSites();
    
    if(ELECTRO)
    {
        getBasicSites();
    }

    /************* STIFFEN SEGMENTS *******************/

    if (STIFFEN) //stiffen only if STIFFEN is 1 in driveM
    {
        initializeStiffSites();
    }

    /************* ELECTRO SEGMENTS *******************/

    if (ELECTRO) //phosphorylate sites only if ELECTRO is 1 in driveM
    {
        initializePhosphorylatedSites();
    }
    
    /********* INITIALIZE CONFIGURATION *******************/

    //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
    //initalize base
	tBase[0]=0;
	tBase[1]=0;
	tBase[2]=1;
	rBase[0]=0;
	rBase[1]=0;
	rBase[2]=0;
	e1Base[0]=1;
	e1Base[1]=0;
	e1Base[2]=0;
	e2Base[0]=0;
	e2Base[1]=1;
	e2Base[2]=0;
	
	// initial configuration
    for(nf=0;nf<NFil;nf++)
    {
        for(i=0;i<N[nf];i++)
        {
            phi[nf][i]   = 0;
            theta[nf][i] = 0;
            psi[nf][i]   = 0;
            
            r[nf][i][0]  = 0;
            r[nf][i][1]  = 0;
            r[nf][i][2]  = i+1;
            
            t[nf][i][0]  = 0;
            t[nf][i][1]  = 0;
            t[nf][i][2]  = 1;
            
            e1[nf][i][0] = 1;
            e1[nf][i][1] = 0;
            e1[nf][i][2] = 0;
            
            e2[nf][i][0] = 0;
            e2[nf][i][1] = 1;
            e2[nf][i][2] = 0;
            
        }
    }
    
    //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
    if (BASEBOUND)
    {
        baseCenter[0] = 0;
        baseCenter[1] = 0;
        baseCenter[2] = -baserLigand;
    }
    
    
    // Initialize bLigandCenter locations
    for(nf=0;nf<NFil;nf++)
    {
        for(ib=0;ib<bSiteTotal[nf];ib++) //for each bound iSite, find the center of the attached ligand
        {
            switch (ib % 4) //currently changes orientation of ligand center based on where it is in list of bound sites
            {
                case 0: //standard orientation - same as used for iSite Pocc calculations
                    
                    bSiteCurrent = bSite[nf][ib];
                    bLigandCenter[nf][ib][0] = r[nf][bSiteCurrent][0] + brLigand*e1[nf][bSiteCurrent][0];
                    bLigandCenter[nf][ib][1] = r[nf][bSiteCurrent][1] + brLigand*e1[nf][bSiteCurrent][1];
                    bLigandCenter[nf][ib][2] = r[nf][bSiteCurrent][2] + brLigand*e1[nf][bSiteCurrent][2];
                    break;
                    
                case 1: //180 degrees from standard
                    
                    bSiteCurrent = bSite[nf][ib];
                    bLigandCenter[nf][ib][0] = r[nf][bSiteCurrent][0] - brLigand*e1[nf][bSiteCurrent][0];
                    bLigandCenter[nf][ib][1] = r[nf][bSiteCurrent][1] - brLigand*e1[nf][bSiteCurrent][1];
                    bLigandCenter[nf][ib][2] = r[nf][bSiteCurrent][2] - brLigand*e1[nf][bSiteCurrent][2];
                    break;
                    
                case 2: //90 degrees from standard
                    
                    bSiteCurrent = bSite[nf][ib];
                    bLigandCenter[nf][ib][0] = r[nf][bSiteCurrent][0] + brLigand*e2[nf][bSiteCurrent][0];
                    bLigandCenter[nf][ib][1] = r[nf][bSiteCurrent][1] + brLigand*e2[nf][bSiteCurrent][1];
                    bLigandCenter[nf][ib][2] = r[nf][bSiteCurrent][2] + brLigand*e2[nf][bSiteCurrent][2];
                    break;
                    
                case 3: //270 from standard
                    
                    bSiteCurrent = bSite[nf][ib];
                    bLigandCenter[nf][ib][0] = r[nf][bSiteCurrent][0] - brLigand*e2[nf][bSiteCurrent][0];
                    bLigandCenter[nf][ib][1] = r[nf][bSiteCurrent][1] - brLigand*e2[nf][bSiteCurrent][1];
                    bLigandCenter[nf][ib][2] = r[nf][bSiteCurrent][2] - brLigand*e2[nf][bSiteCurrent][2];
                    break;
            }
            
        }
    }
    
    
    
    
	convergedTF=0;
	nt=0;
    E = INF;
    Eelectro = INF;

    //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
    // stuff needed for automatic perturbation size adaptation
	for(iParam=0;iParam<2;iParam++)
	{
		dChi[iParam] = DCHIINIT;
		proposals[iParam] = 0;
		accepts[iParam] = 0;
	}
    
    // stuff needed for automatic stationarity (convergence) check
	ntNextStationarityCheck = 3*NTCHECK;
	for(iBin=0;iBin<NBINS;iBin++)
		convergenceVariableCounts[iBin] = 0;
	
    // summary variables
    initializeSummary();

    /********* BEGIN LOOP THROUGH ITERATIONS! *******************/
	while(!convergedTF && nt < NTMAX) // Time loop!
    {

        // adapt step size
        if (!(nt % NTADAPT))
        {
            //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
            rate[0] = (double)accepts[0]/(double)proposals[0];
            rate[1] = (double)accepts[1]/(double)proposals[1];
            
            //printf("current accepts/proposals: %d/%d=%f, %d/%d=%f\n", accepts[0], proposals[0], rate[0], accepts[1], proposals[1], rate[1]);
            
            for(iParam=0;iParam<2;iParam++)
            {
                accepts[iParam] = 0;
                proposals[iParam] = 0;
                if (nt < NTCHECK)
                {
                    if (rate[iParam] > 0.5 || rate[iParam] < 0.3)
                    {
                        dChi[iParam] = dChi[iParam]*rate[iParam]/0.44;
                        if (dChi[iParam] > PI/4)
                            dChi[iParam] = PI/4;
                        if (dChi[iParam] < DCHIMIN)
                            dChi[iParam] = DCHIMIN;
                    }
                }
            }
            
        } // done adapting step

        /********* OUTLINE OF ALGORITHM *******************/
        // 1. We create a new proposal configuration and
        // then decide:
        // 2. whether the proposal configuration satisfies any constraints (e.g., floor?), and
        // 3. whether the proposal configuration is accepted by Metropolis test (always accept for a freely-jointed chain)
        // After the configuration update, we:
        // 4. collect any data of interest (e.g., ree; whether there is steric occlusion), and write it to file
        // 5. test for stationarity (convergence)
        // 6. increment the iteration and repeat
		
        /****************************************************************/
        /********* 1. Generate proposal configuration *******************/
        /****************************************************************/
        for(nf=0;nf<NFil;nf++)
        {
            for(i=0;i<N[nf];i++)
            {
                phiPropose[nf][i]   = phi[nf][i];
                thetaPropose[nf][i] = theta[nf][i];
                psiPropose[nf][i]   = psi[nf][i];
            }
        }
		
        nfPropose = floor(NFil*TWISTER); // Initialize. This is the filament we will adjust this time.
		iPropose = floor(N*TWISTER); // Initialize. This is the joint we will adjust this time.
        

        //If using STIFFEN
            //test if joint is stiff
            //replace proposed joint until one is found that is not stiff
        if (STIFFEN)
        {
            while(StiffSites[nfPropose][iPropose]==1) //Test if proposed joint is stiff.
            {
                nfPropose = floor(NFil*TWISTER); //If stiff, propose new filament and joint until propose one not stiff.
                iPropose = floor(N*TWISTER); //If stiff, propose new joint until propose one not stiff.
            }
        }
        
        //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
		if(iPropose==0)
		{
			dChiHere = dChi[0];
			proposals[0]++;
		}
		else 
		{
			dChiHere = dChi[1];
			proposals[1]++;
		}
		
        constraintProposalsTotal = 0;
        long rounds = 0;
		constraintSatisfiedTF = 0;

		while(!constraintSatisfiedTF && constraintProposalsTotal < CPMAX) // keep generating proposal configurations until we get one that satisfies the constraint
        {
            iChi = floor(3*TWISTER);
				
            //printf("iChi=%d, iPropose=%d\n", iChi, iPropose);
				
            // propose new configuration angles
            // We can use normal (Gaussian) random variables using the Box-Muller method (see eg wikipedia)
            // or uniform random variable
            switch (iChi)
            {
                case 0:
                    //phiPropose[nfPropose][iPropose]   = phiPropose[nfPropose][iPropose]   + dChiHere*sqrt(-2*log(TWISTER))*cos(2*PI*TWISTER);
                    phiPropose[nfPropose][iPropose]   = phiPropose[nfPropose][iPropose]   + dChiHere*(2*TWISTER-1);
                    break;
                case 1:
                    //thetaPropose[nfPropose][iPropose] = thetaPropose[nfPropose][iPropose] + dChiHere*sqrt(-2*log(TWISTER))*cos(2*PI*TWISTER);
                    thetaPropose[nfPropose][iPropose] = thetaPropose[nfPropose][iPropose] + dChiHere*(2*TWISTER-1);
                    break;
                case 2:
                    //psiPropose[nfPropose][iPropose]   = psiPropose[nfPropose][iPropose]   + dChiHere*sqrt(-2*log(TWISTER))*cos(2*PI*TWISTER);
                    psiPropose[nfPropose][iPropose]   = psiPropose[nfPropose][iPropose]   + dChiHere*(2*TWISTER-1);
                    break;
            }

            // -- translate to proposal configuration --
            // rotate base
            
            
            //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
            rotate(&tBase[0], &e1Base[0], &e2Base[0], &tPropose[nf][0][0], &e1Propose[nf][0][0], &e2Propose[nf][0][0], phiPropose[nf][0], thetaPropose[nf][0], psiPropose[nf][0]);
            
            //THINK ABOUT THIS PART HERE!!!!!!!!!!!!!
            for(ix=0;ix<3;ix++)
                rPropose[nf][0][ix] = rBase[ix] + tPropose[nf][0][ix];
				
            // if nf != nfPropose, proposal configuration is the same as current configuration
            // if nf = nfPropose and i<iPropose, proposal configuration is the same as current configuration
            for(nf=0;nf<NFil;nf++)
            {
                if(nf==nfPropose) // if proposed filament, only keep segments before iPropose the same
                {
                    for(i=1;i<iPropose;i++)
                    {
                        rPropose[nf][i][0] = r[nf][i][0];
                        rPropose[nf][i][1] = r[nf][i][1];
                        rPropose[nf][i][2] = r[nf][i][2];
                        
                        tPropose[nf][i][0] = t[nf][i][0];
                        tPropose[nf][i][1] = t[nf][i][1];
                        tPropose[nf][i][2] = t[nf][i][2];
                        
                        e1Propose[nf][i][0] = e1[nf][i][0];
                        e1Propose[nf][i][1] = e1[nf][i][1];
                        e1Propose[nf][i][2] = e1[nf][i][2];
                        
                        e2Propose[nf][i][0] = e2[nf][i][0];
                        e2Propose[nf][i][1] = e2[nf][i][1];
                        e2Propose[nf][i][2] = e2[nf][i][2];
                    }
                }
                else{ //if not proposed filament, keep all segments the same
                    for(i=1;i<N[nf];i++)
                    {
                        rPropose[nf][i][0] = r[nf][i][0];
                        rPropose[nf][i][1] = r[nf][i][1];
                        rPropose[nf][i][2] = r[nf][i][2];
                        
                        tPropose[nf][i][0] = t[nf][i][0];
                        tPropose[nf][i][1] = t[nf][i][1];
                        tPropose[nf][i][2] = t[nf][i][2];
                        
                        e1Propose[nf][i][0] = e1[nf][i][0];
                        e1Propose[nf][i][1] = e1[nf][i][1];
                        e1Propose[nf][i][2] = e1[nf][i][2];
                        
                        e2Propose[nf][i][0] = e2[nf][i][0];
                        e2Propose[nf][i][1] = e2[nf][i][1];
                        e2Propose[nf][i][2] = e2[nf][i][2];
                    }
                }
            }
            
            // rotate all segments above (and including) iPropose
            
            if (iPropose==0)
                iStart=1;
            else
                iStart=iPropose;
            for(i=iStart;i<N[nfPropose];i++)
            {
                rotate(&tPropose[nf][i-1][0], &e1Propose[nf][i-1][0], &e2Propose[nf][i-1][0], &tPropose[nf][i][0], &e1Propose[nf][i][0], &e2Propose[nf][i][0],
                    phiPropose[nf][i], thetaPropose[nf][i], psiPropose[nf][i]);
                for (ix=0;ix<3;ix++)
                    rPropose[nf][i][ix] = rPropose[nf][i-1][ix] + tPropose[nf][i][ix];
            }
            
            //how does this work?  only returns first component of t, e1, e2? where are the other components?

            
            // Set bound ligand center proposals based on new filament locations
            for(nf=0;nf<NFil;nf++)
            {
                for(ib=0;ib<bSiteTotal[nf];ib++) //for each bound iSite, find the center of the attached ligand
                {
                    switch (ib % 4) //currently changes orientation of ligand center based on where it is in list of bound sites
                    {
                        case 0: //standard orientation - same as used for iSite Pocc calculations
                            
                            bSiteCurrent = bSite[nf][ib];
                            bLigandCenterPropose[nf][ib][0] = rPropose[nf][bSiteCurrent][0] + brLigand*e1Propose[nf][bSiteCurrent][0];
                            bLigandCenterPropose[nf][ib][1] = rPropose[nf][bSiteCurrent][1] + brLigand*e1Propose[nf][bSiteCurrent][1];
                            bLigandCenterPropose[nf][ib][2] = rPropose[nf][bSiteCurrent][2] + brLigand*e1Propose[nf][bSiteCurrent][2];
                            break;
                            
                        case 1: //180 degrees from standard
                            
                            bSiteCurrent = bSite[nf][ib];
                            bLigandCenterPropose[nf][ib][0] = rPropose[nf][bSiteCurrent][0] - brLigand*e1Propose[nf][bSiteCurrent][0];
                            bLigandCenterPropose[nf][ib][1] = rPropose[nf][bSiteCurrent][1] - brLigand*e1Propose[nf][bSiteCurrent][1];
                            bLigandCenterPropose[nf][ib][2] = rPropose[nf][bSiteCurrent][2] - brLigand*e1Propose[nf][bSiteCurrent][2];
                            break;
                            
                        case 2: //90 degrees from standard
                            
                            bSiteCurrent = bSite[nf][ib];
                            bLigandCenterPropose[nf][ib][0] = rPropose[nf][bSiteCurrent][0] + brLigand*e2Propose[nf][bSiteCurrent][0];
                            bLigandCenterPropose[nf][ib][1] = rPropose[nf][bSiteCurrent][1] + brLigand*e2Propose[nf][bSiteCurrent][1];
                            bLigandCenterPropose[nf][ib][2] = rPropose[nf][bSiteCurrent][2] + brLigand*e2Propose[nf][bSiteCurrent][2];
                            break;
                            
                        case 3: //270 from standard
                            
                            bSiteCurrent = bSite[nf][ib];
                            bLigandCenterPropose[nf][ib][0] = rPropose[nf][bSiteCurrent][0] - brLigand*e2Propose[nf][bSiteCurrent][0];
                            bLigandCenterPropose[nf][ib][1] = rPropose[nf][bSiteCurrent][1] - brLigand*e2Propose[nf][bSiteCurrent][1];
                            bLigandCenterPropose[nf][ib][2] = rPropose[nf][bSiteCurrent][2] - brLigand*e2Propose[nf][bSiteCurrent][2];
                            break;
                    }
                }
            }
            

            /****************************************************************/
            /******************* 2. Test constraints ************************/
            /****************************************************************/
            
            constraintSatisfiedTF=1;
            
            //check if hard membrane occludes polymer (when not using ELECTRO)
            if (MEMBRANE && !ELECTRO)
            {
                //printf("Testing Membrane");

                for(nf=0;nf<NFil;nf++)
                {
                    for(i=0;i<N[nf];i++)
                    {
                        if (rPropose[nf][i][2] < 0)
                        {
                            constraintSatisfiedTF=0;
                            i=N[nf]+1; // shortcut out of the loop
                            nf = NFil; // shortcut out of outer loop
                            //printf("Membrane constraint failed.");
                        }
                    }
                } // done checking constraint
                
            } //finished first constraint
            
            // check if BASEBOUND (immobile sphere at base) occludes polymer
            if (BASEBOUND)
            {
                for(nf=0;nf<NFil;nf++)
                {
                    for(i=0;i<N[nf];i++)// for each joint
                    {
                        //test polymer against sphere at base
                        if ( ((baseCenter[0]-rPropose[nf][i][0])*(baseCenter[0]-rPropose[nf][i][0]) +
                              (baseCenter[1]-rPropose[nf][i][1])*(baseCenter[1]-rPropose[nf][i][1]) +
                              (baseCenter[2]-rPropose[nf][i][2])*(baseCenter[2]-rPropose[nf][i][2]) <= baserLigand*baserLigand )) //if proposed joint is inside base ligand sphere
                        {
                            constraintSatisfiedTF=0; //constraint not satisfied
                            i=N[nf]; //shortcut out of inner loop
                        }
                    }
                }
            } // finished second constraint

            //only test if looking at multiple binding and if membrane constraint passed
            if (MULTIPLE && constraintSatisfiedTF)
            {
                
                //printf("Testing bound ligands.");
                for(nf=0;nf<NFil;nf++) //for each filament
                {
                    for (ib=0;ib<bSiteTotal[nf];ib++) //for each bound ligand on filament
                    {
                        
                       if (MEMBRANE)
                       {
                            if(bLigandCenterPropose[nf][ib][2]<brLigand) // if any bound ligands intersect with membrane
                            {
                                constraintSatisfiedTF = 0; //constraint not satisfied
                                ib = bSiteTotal[nf]; //shortcut out of middle loop
                                nf = NFil; //shortcut out of outer loop
                            }
                       }
                        
                       if(constraintSatisfiedTF) //if passed membrane constraint, test joints
                       {
                           for(nf2=nf;nf2<NFil;nf2++)
                           {
                                for(i=0;i<N[nf2];i++)// for each joint
                                {
                                    if ( ((bLigandCenterPropose[nf][ib][0]-rPropose[nf2][i][0])*(bLigandCenterPropose[nf][ib][0]-rPropose[nf2][i][0]) +
                                        (bLigandCenterPropose[nf][ib][1]-rPropose[nf2][i][1])*(bLigandCenterPropose[nf][ib][1]-rPropose[nf2][i][1]) +
                                        (bLigandCenterPropose[nf][ib][2]-rPropose[nf2][i][2])*(bLigandCenterPropose[nf][ib][2]-rPropose[nf2][i][2]) <= brLigand*brLigand )
                                        && !( nf2 == nf && i == bSite[nf][ib]) ) //if proposed joint is inside ligand sphere AND joint is not where tested ligand is attached
                                    {
                                        constraintSatisfiedTF=0; //constraint not satisfied
                                        i=N[nf2]; //shortcut out of inner loop
                                        nf2 = NFil; //shortcut out of middle loop
                                        ib=bSiteTotal[nf];// shortcut out of outer loop
                                        nf = NFil;// shortcut out of outer most loop
                                    }
                                }
                           }
                        }
                        
                        // only have 1 base ligand for all filaments
                        if (constraintSatisfiedTF && BASEBOUND) //if constraint is still satisfied, test ligand sphere with base ligand if exists
                        {
                            if ((bLigandCenterPropose[nf][ib][0]-baseCenter[0])*(bLigandCenterPropose[nf][ib][0]-baseCenter[0])+
                                (bLigandCenterPropose[nf][ib][1]-baseCenter[1])*(bLigandCenterPropose[nf][ib][1]-baseCenter[1])+
                                (bLigandCenterPropose[nf][ib][2]-baseCenter[2])*(bLigandCenterPropose[nf][ib][2]-baseCenter[2])<=
                                (brLigand+baserLigand)*(brLigand+baserLigand)) //if distance between centers is less than brLigand+baserLigand, then ligands are intersecting
                            {
                                constraintSatisfiedTF=0; //constraint not satisfied
                                ib=bSiteTotal[nf];// shortcut out of outer loop
                                nf = NFil; //shortcut out of outer most loop
                            }
                            
                         }
                        

                         if (constraintSatisfiedTF) //if constraint is still satisfied, test ligand sphere with other ligands on filaments
                         {
                             // check ligand against other ligands on filaments
                             for(nf2=nf;nf2<NFil;nf2++) //look at this filament and all following filaments
                             {
                                for (ib2=0;ib2<bSiteTotal[nf2];ib2++) //for each next ligand
                                {
                                    
                                    if ((bLigandCenterPropose[nf][ib][0]-bLigandCenter[nf2][ib2][0])*(bLigandCenterPropose[nf][ib][0]-bLigandCenter[nf2][ib2][0]) +
                                        (bLigandCenterPropose[nf][ib][1]-bLigandCenter[nf2][ib2][1])*(bLigandCenterPropose[nf][ib][1]-bLigandCenter[nf2][ib2][1]) +
                                        (bLigandCenterPropose[nf][ib][2]-bLigandCenter[nf2][ib2][2])*(bLigandCenterPropose[nf][ib][2]-bLigandCenter[nf2][ib2][2])<=
                                        (2*brLigand)*(2*brLigand) && bSite[nf][ib]!= bSite[nf2][ib2]) //if distance between centers is less than 2*brLigand, then ligands are intersecting, && bound ligands being compared are not the same ligand
                                    {
                                        constraintSatisfiedTF=0; //constraint not satisfied
                                        ib2=bSiteTotal[nf2]; //shortcut out of loop
                                        nf2 = NFil; //shortcut out of middle loop
                                        ib=bSiteTotal[nf]; //shortcut out of outer loop
                                        nf = NFil; //shortcut out of outer most loop
                                    }
                                 }
                             }
                          }
                        }
                }

                } //finished last constraint
            
            constraintProposalsTotal++; //count number of times proposals are rejected this time step
        } //finish constraint while loop
        
        if (constraintProposalsTotal >= CPMAX) //if number of proposals exceeds CPMAX, exit program
        {
            printf("Exceeded maximum proposals.\n");
            fflush(stdout);
            
            exit(0);
        }
        
        /****************************************************************/
        /********************* 3. Metropolis test ***********************/
        /****************************************************************/
        // We now have a propsoal configuration that passes the constraints.
        // Step 3 is to see if it passes our acceptance test (Metropolis test).
        
        if (!ELECTRO)
        {
            // Compute energy
            for(nf=0;nf<NFil;nf++)
            {
                ENew = -rPropose[nf][N-1][2]*Force; // Energy in units of kBT. Force in units of kBT/Kuhn
            }
            
            // should this be <=? Do we reject normal probability for no force?
            if (  TWISTER < exp(E-ENew) ) //always accepts if ENew<E, accepts with normal (?) probability if ENew>E
            {
                E = ENew;
                
                // Make configuration into the proposal configuration
                for(i=iPropose;i<N[nfPropose];i++)
                {
                    phi[nfPropose][i]   = phiPropose[nfPropose][i];
                    theta[nfPropose][i] = thetaPropose[nfPropose][i];
                    psi[nfPropose][i]   = psiPropose[nfPropose][i];
                    
                    r[nfPropose][i][0] = rPropose[nfPropose][i][0];
                    r[nfPropose][i][1] = rPropose[nfPropose][i][1];
                    r[nfPropose][i][2] = rPropose[nfPropose][i][2];
                    
                    t[nfPropose][i][0] = tPropose[i][0];
                    t[nfPropose][i][1] = tPropose[i][1];
                    t[nfPropose][i][2] = tPropose[i][2];
                    
                    e1[nfPropose][i][0] = e1Propose[nfPropose][i][0];
                    e1[nfPropose][i][1] = e1Propose[nfPropose][i][1];
                    e1[nfPropose][i][2] = e1Propose[nfPropose][i][2];
                    
                    e2[nfPropose][i][0] = e2Propose[nfPropose][i][0];
                    e2[nfPropose][i][1] = e2Propose[nfPropose][i][1];
                    e2[nfPropose][i][2] = e2Propose[nfPropose][i][2];
                 
                }
                
                if(MULTIPLE)
                {
                    //accept bound ligand configurations
                    for(nf=0;nf<NFil;nf++)
                    {
                        for(ib=0;ib<bSiteTotal[nf];ib++)
                        {
                            bLigandCenter[nf][ib][0] = bLigandCenterPropose[nf][ib][0];
                            bLigandCenter[nf][ib][1] = bLigandCenterPropose[nf][ib][1];
                            bLigandCenter[nf][ib][2] = bLigandCenterPropose[nf][ib][2];
                            
                        }
                    }
                }
                
                if(iPropose==0)
                    accepts[0] ++;
                else 		
                    accepts[1] ++;
                
            }
        }
        else
        {
 
            //sum over energies of all joints, except phosphorylated ones
            EelectroNew = 0;

                // create electrostatic potential
                // 1. Basic residues feel parabolic potential
                // 2. Tyrosines feel same potential as rest of amino acids (hardwall or softwall)
                // 3. Phosphorylated tyrosines feel negative potential (repulsion from membrane)
            for(nf=0;nf<NFil;nf++)
            {
                for (i=0;i<N[nf];i++)
                {
                    // if basic and not phosphorylated
                    if((BasicSitesYN[nf][i]==1)&&(PhosphorylatedSites[nf][i]!=1))
                    {
                        if(rPropose[nf][i][2]<(sqrt(parabolaDepth/parabolaWidth)))
                        {
                        // Compute energy
                            EelectroNew += parabolaWidth*(rPropose[nf][i][2])*(rPropose[nf][i][2])-parabolaDepth;
                        }
                        
                    }
                    else
                    {
                        // if phosphorylated
                        if( PhosphorylatedSites[nf][i]==1 )
                        {
                            if (HARDWALL) //hard wall for phosphorylated tyrosines too
                            {
                                if (rPropose[nf][i][2]>0)
                                {
                                    // repulsive force with E*e^(-z/zbar)
                                    EelectroNew += Erepulsion*(exp(-rPropose[nf][i][2]/Zrepulsion));
                                }
                                else
                                {
                                    // hardwall at 0 for phosphorylated tyrosines - probably unnecessary
                                    EelectroNew += INF;
                                }
                            }
                            else //no hardwall for phosphorylated tyrosines - use exponential
                            {
                                // repulsive force with E*e^(-z/zbar)
                                EelectroNew += Erepulsion*(exp(-rPropose[nf][i][2]/Zrepulsion));
                            }

                        }
                        else //if anything else (tyrosine, other amino acid)
                        {
                            if (HARDWALL) // hard wall
                            {
                                if(rPropose[nf][i][2]<=0)
                                {
                                    // Compute energy
                                    EelectroNew += INF;
                                }
                                
                            }
                            else //soft wall
                            {
                                if(rPropose[nf][i][2]<=0)
                                {
                                    // Compute energy
                                    EelectroNew += wallParabolaK*(rPropose[nf][i][2])*(rPropose[nf][i][2]);
                                }
                            }
                          }
                        }
                    }
                }

            if (  TWISTER < exp(Eelectro-EelectroNew) ) //always accepts if ENew<E, accepts with normal (?) probability if ENew>E
            {

                Eelectro = EelectroNew;

                // Make configuration into the proposal configuration
                for(i=iPropose;i<N[nfPropose];i++)
                {
                    phi[nfPropose][i]   = phiPropose[nfPropose][i];
                    theta[nfPropose][i] = thetaPropose[nfPropose][i];
                    psi[nfPropose][i]   = psiPropose[nfPropose][i];
                    
                    r[nfPropose][i][0] = rPropose[nfPropose][i][0];
                    r[nfPropose][i][1] = rPropose[nfPropose][i][1];
                    r[nfPropose][i][2] = rPropose[nfPropose][i][2];
                    
                    t[nfPropose][i][0] = tPropose[nfPropose][i][0];
                    t[nfPropose][i][1] = tPropose[nfPropose][i][1];
                    t[nfPropose][i][2] = tPropose[nfPropose][i][2];
                    
                    e1[nfPropose][i][0] = e1Propose[nfPropose][i][0];
                    e1[nfPropose][i][1] = e1Propose[nfPropose][i][1];
                    e1[nfPropose][i][2] = e1Propose[nfPropose][i][2];
                    
                    e2[nfPropose][i][0] = e2Propose[nfPropose][i][0];
                    e2[nfPropose][i][1] = e2Propose[nfPropose][i][1];
                    e2[nfPropose][i][2] = e2Propose[nfPropose][i][2];
                    
                }
                
                if(MULTIPLE)
                {
                    //accept bound ligand configurations
                    for(nf=0;nf<NFil;nf++)
                    {
                        for(ib=0;ib<bSiteTotal[nf];ib++)
                        {
                            bLigandCenter[nf][ib][0] = bLigandCenterPropose[nf][ib][0];
                            bLigandCenter[nf][ib][1] = bLigandCenterPropose[nf][ib][1];
                            bLigandCenter[nf][ib][2] = bLigandCenterPropose[nf][ib][2];
                            
                        }
                    }
                }
                if(iPropose==0)
                accepts[0] ++;
                else
                accepts[1] ++;
                
             }

            } // end configuration loop
        
        /**************************************************************************/
        /**************** 4. Data collection and output to file *******************/
        /**************************************************************************/
        
        // check if blocking sphere
        if (1)
        {
            /***********************************/
            /*******Initialize variables********/
            /***********************************/
            
            for(iy=0;iy<iSiteTotal;iy++) //for each iSite, determine the center of the ligand sphere
            {
                iSiteCurrent = iSite[iy];
                iLigandCenter[iy][0] = r[iSiteCurrent][0] + irLigand*e1[iSiteCurrent][0];
                iLigandCenter[iy][1] = r[iSiteCurrent][1] + irLigand*e1[iSiteCurrent][1];
                iLigandCenter[iy][2] = r[iSiteCurrent][2] + irLigand*e1[iSiteCurrent][2];
            
                stericOcclusion[iy]             = 0; //set steric occlusion array to 0 for each iSite
                membraneOcclusion[iy]           = 0; //set membrane occlusion array to 0 for each iSite
                membraneAndSegmentOcclusion[iy] = 0; //set membrane and segment occlusion array to 0 for each iSite
            }
            
            //initialize ligand center at base
            baseLigandCenter[0] = rBase[0] + irLigand*e1Base[0];
            baseLigandCenter[1] = rBase[1] + irLigand*e1Base[1];
            baseLigandCenter[2] = rBase[2] + irLigand*e1Base[2];
            
            //initialize steric occlusion at base to 0
            stericOcclusionBase = 0;
            
            if(MULTIPLE)
            {
                //Is it better to write over bLigandCenter or to reinitialize or to create bLigandCenterPropose and copy it over when it passes?
                //determine center of bound ligand sphere, maintaining orientation as in proposals above
                for(ib=0;ib<bSiteTotal;ib++) //for each bound iSite, find the center of the attached ligand
                {
                    switch (ib % 4) //currently changes orientation of ligand center based on where it is in list of bound sites
                    {
                        case 0: //standard orientation - same as used for iSite Pocc calculations
                            
                            bSiteCurrent = bSite[ib];
                            bLigandCenter[ib][0] = r[bSiteCurrent][0] + brLigand*e1[bSiteCurrent][0];
                            bLigandCenter[ib][1] = r[bSiteCurrent][1] + brLigand*e1[bSiteCurrent][1];
                            bLigandCenter[ib][2] = r[bSiteCurrent][2] + brLigand*e1[bSiteCurrent][2];
                            break;
                            
                        case 1: //180 degrees from standard
                            
                            bSiteCurrent = bSite[ib];
                            bLigandCenter[ib][0] = r[bSiteCurrent][0] - brLigand*e1[bSiteCurrent][0];
                            bLigandCenter[ib][1] = r[bSiteCurrent][1] - brLigand*e1[bSiteCurrent][1];
                            bLigandCenter[ib][2] = r[bSiteCurrent][2] - brLigand*e1[bSiteCurrent][2];
                            break;
                            
                        case 2: //90 degrees from standard
                            
                            bSiteCurrent = bSite[ib];
                            bLigandCenter[ib][0] = r[bSiteCurrent][0] + brLigand*e2[bSiteCurrent][0];
                            bLigandCenter[ib][1] = r[bSiteCurrent][1] + brLigand*e2[bSiteCurrent][1];
                            bLigandCenter[ib][2] = r[bSiteCurrent][2] + brLigand*e2[bSiteCurrent][2];
                            break;
                            
                        case 3: //270 from standard
                            
                            bSiteCurrent = bSite[ib];
                            bLigandCenter[ib][0] = r[bSiteCurrent][0] - brLigand*e2[bSiteCurrent][0];
                            bLigandCenter[ib][1] = r[bSiteCurrent][1] - brLigand*e2[bSiteCurrent][1];
                            bLigandCenter[ib][2] = r[bSiteCurrent][2] - brLigand*e2[bSiteCurrent][2];
                            break;
                    }
                    
                }
            }
            
            
            /*******************************************/
            /**********Test Occlusion of iSites*********/
            /*******************************************/
            
            //tests occlusion of iSites first
            for(iy=0; iy<iSiteTotal;iy++)
            {
                
                // test if iSite is already bound
                if (MULTIPLE)
                {
                    for (ib=0;ib<bSiteTotal;ib++)
                    {
                        if(iSite[iy]==bSite[ib]) //test if iSite is bound already
                        {
                            stericOcclusion[iy]++;
                            membraneAndSegmentOcclusion[iy]++;
                            ib=bSiteTotal;
                        }
                    }//didn't include base - assuming can't be bound to base
                }
                
                //test if iSite occluded by membrane
                if (stericOcclusion[iy]==0) //if not occluded yet, do further tests
                {
                    if (MEMBRANE)
                    {
                        // check if sphere violates membrane
                        if (iLigandCenter[iy][2]<irLigand)
                        {
                            stericOcclusion[iy]++; //sterically occluded
                            membraneOcclusion[iy]++; //specifically occluded by the membrane
                            membraneAndSegmentOcclusion[iy]++; //occluded by membrane or polymer segments (NOT other bound molecules)
                        }
                    }
                    else
                    {
                        // check if sphere violates base (end point of polymer - origin)
                        if ( (iLigandCenter[iy][0])*(iLigandCenter[iy][0]) +
                             (iLigandCenter[iy][1])*(iLigandCenter[iy][1]) +
                             (iLigandCenter[iy][2])*(iLigandCenter[iy][2]) <= irLigand*irLigand )
                        {
                            stericOcclusion[iy]++;
                            membraneAndSegmentOcclusion[iy]++;
                        }
                    
                        //didn't include base - don't want the base to violate the base
                    } // finished membrane tests
                }
                
                if (stericOcclusion[iy]==0) //if not occluded yet, do further tests
                {
                    for(i=0;i<N;i++) // loop through joints
                    {
                        if ( (iLigandCenter[iy][0]-r[i][0])*(iLigandCenter[iy][0]-r[i][0]) +
                             (iLigandCenter[iy][1]-r[i][1])*(iLigandCenter[iy][1]-r[i][1]) +
                             (iLigandCenter[iy][2]-r[i][2])*(iLigandCenter[iy][2]-r[i][2]) <= irLigand*irLigand
                            && i != iSite[iy])
                        {
                            stericOcclusion[iy]++;
                            membraneAndSegmentOcclusion[iy]++;
                            i=N; // shortcut out of the loop
                        }
                    } // finished loop through joints

                } 
                
                if (stericOcclusion[iy]==0) //if not occluded yet, do further tests
                {
                    if (MULTIPLE) //if there are multiple ligands and not occluded yet, test other ligands
                    {
                        for(ib=0;ib<bSiteTotal;ib++) //for each bound iSite, find the center of the attached ligand
                        {
                            
                            if ( (iLigandCenter[iy][0]-bLigandCenter[ib][0])*(iLigandCenter[iy][0]-bLigandCenter[ib][0]) +
                                 (iLigandCenter[iy][1]-bLigandCenter[ib][1])*(iLigandCenter[iy][1]-bLigandCenter[ib][1]) +
                                 (iLigandCenter[iy][2]-bLigandCenter[ib][2])*(iLigandCenter[iy][2]-bLigandCenter[ib][2]) <= (irLigand+brLigand)*(irLigand+brLigand))
                                // if potential ligand intersects with bound ligand
                            {
                                stericOcclusion[iy]++;
                                ib=bSiteTotal; //shortcut out of the loop
                            }
                        }
                    } // finished multiple ligand tests
                }
                
                if (stericOcclusion[iy]==0)
                {
                    if (BINDTRANSITION && nt > NTCHECK) // if allow code to transition between unbound and bound ligands and iSite is unoccluded and past initial transient
                    {
                        bSiteTotal++; // increase number of bound ligands by 1
                        bSite[bSiteTotal-1] = iSite[iy]; //make iSite into bSite
                        
                        if(1)
                        {
                            printf("Adding bound ligand at iSite[%ld] = %ld! \n",iy,iSite[iy]);
                            fflush(stdout);
                            printf("Number of bound ligands: %ld \n",bSiteTotal);
                            fflush(stdout);
                        }
                    }
                }
            } // finished loop through iSites
        
        
            /**********************************************/
            /***********Check Occlusion of Base************/
            /**********************************************/
            
            if (!MEMBRANE) //only check occlusion if there is no membrane (membrane implies always occluded at base)
            {
                
                /*****Initialize******/
                //initialize ligand center and stericOcclusion
                baseLigandCenter[0] = rBase[0] + irLigand*e1Base[0];
                baseLigandCenter[1] = rBase[1] + irLigand*e1Base[1];
                baseLigandCenter[2] = rBase[2] + irLigand*e1Base[2];
                
                stericOcclusionBase = 0;

                
                /*****Test Occlusion of Base*****/
                //check occlusion with joints
                for(i=0;i<N;i++)
                {
                    if ( (baseLigandCenter[0]-r[i][0])*(baseLigandCenter[0]-r[i][0]) +
                        (baseLigandCenter[1]-r[i][1])*(baseLigandCenter[1]-r[i][1]) +
                        (baseLigandCenter[2]-r[i][2])*(baseLigandCenter[2]-r[i][2]) <= irLigand*irLigand)
                    {
                        stericOcclusionBase+=N;
                        i=N; // shortcut out of the loop
                    }
                }
                
                //check occlusion with other ligands  - to test if they "deliver" their cargo
                //or do we want to test Occlusion of base with ligands? Could do both?
                if (MULTIPLE && stericOcclusionBase==0)
                {
//                    //initialize
//                    for (ib=0;ib<bSiteTotal;ib++)
//                    {
//                        boundToBaseDeliver[ib]=0;
//                    }
                    
                    
//                    switch (deliveryMethod)
//                    {
//                            case 0:
                                //for each bound iSite, test if bound ligand intersects with base ligand site
                    for (ib=0; ib<bSiteTotal;ib++)
                    {
                       if ((baseLigandCenter[0]-bLigandCenter[ib][0])*(baseLigandCenter[0]-bLigandCenter[ib][0])+(baseLigandCenter[1]-bLigandCenter[ib][1])*(baseLigandCenter[1]-bLigandCenter[ib][1])+(baseLigandCenter[2]-bLigandCenter[ib][2])*(baseLigandCenter[2]-bLigandCenter[ib][2]) <= (irLigand+brLigand)*(irLigand+brLigand))
                          {
                              //boundToBaseDeliver[ib]++;
                              stericOcclusionBase++;
                           }
                     } // finished loop through bSites
                    
                            //break;
                            
//                            case 1:
//
//                                //for each bound iSite, test if bound ligand is within "delivery" distance
//                                for (ib=0;ib<bSiteTotal;ib++)
//                                {
//                                    if ((bLigandCenter[ib][0])*(bLigandCenter[ib][0])+(bLigandCenter[ib][1])*(bLigandCenter[ib][1])+(bLigandCenter[ib][2])*(bLigandCenter[ib][2]) <= deliveryDistance)
//                                    {
//                                        boundToBaseDeliver[ib]++;
//                                    }
//                                    
//                                    //test if bound ligand intersects base site
//                                    if ((baseLigandCenter[0]-bLigandCenter[ib][0])*(baseLigandCenter[0]-bLigandCenter[ib][0])+(baseLigandCenter[1]-bLigandCenter[ib][1])*(baseLigandCenter[1]-bLigandCenter[ib][1])+(baseLigandCenter[2]-bLigandCenter[ib][2])*(baseLigandCenter[2]-bLigandCenter[ib][2]) <= (irLigand+brLigand)*(irLigand+brLigand))
//                                    {
//                                        stericOcclusionBase++;
//                                    }
//                                }
//                            break;
//                     }
                } // finished checking bound ligands occlusion of base
                
            }//end checking occlusion of base
        
        } //end checking occlusion

		if (1)
		{
            /********* 5. Test for stationarity *******************/
            // This test for stationarity automatically stops the iterations when the distribution is converged to within the tolerance KSCRITICAL.
            if (nt == 2*NTCHECK)
				appendBins();
			
			if (nt == ntNextStationarityCheck)
				stationarity();
		}
		
        // output to time series file
		dataRecording();
        
        /********* 6. Increment time *******************/
		nt++;
		
	} // done time loop
    
    // finalize summary statistics
    finalizeSummary();

}



/********************************************************************************************************/
// Test for convergence ("stationarity")
void stationarity()
{
	double cdf1, cdf2;
	ksStatistic = 0;
	cdf1=0; 
	cdf2=0;
	for(iBin=0;iBin<NBINS;iBin++)
	{
		
		cdf1 += (double)convergenceVariableCountsPrevious[iBin]/(((double)nt-(double)NTCHECK)/2.0);
		cdf2 += (double)convergenceVariableCounts[iBin]/(((double)nt-(double)NTCHECK)/2.0);
		
		if (fabs(cdf1-cdf2)>ksStatistic)
			ksStatistic = fabs(cdf1-cdf2);
		//printf("convergenceVariableCounts[%d]: %d, %d \t\t\t cdf: %f, %f\n", iBin, convergenceVariableCountsPrevious[iBin],convergenceVariableCounts[iBin], cdf1, cdf2);
		
	}
	
	//printf("ksStatistic=%f\n", ksStatistic);
	
	if (ksStatistic<KSCRITICAL)
		convergedTF=1;
	
	appendBins();
	
	ntNextStationarityCheck = 2*(ntNextStationarityCheck-NTCHECK)+NTCHECK;
	return;
}

// Helper function needed by test for convergence ("stationarity")
void appendBins()
{
    for(iBin=0;iBin<NBINS;iBin++)
    {
        convergenceVariableCountsPrevious[iBin] += convergenceVariableCounts[iBin];
        convergenceVariableCounts[iBin] = 0;
    }
}

/********************************************************************************************************/
// This function rotates the orthogonal vectors tIn, e1In and e2In by angles (phiHere, thetaHere, psiHere) in their own frame of reference
void rotate(double *tIn, double *e1In, double *e2In, double *tOut, double *e1Out, double *e2Out, double phiHere, double thetaHere,double psiHere)
{	
	// R Local
    RLocal[0][0] =   cos(thetaHere)*cos(psiHere);
    RLocal[0][1] =   cos(phiHere)*sin(psiHere)     + sin(phiHere)*sin(thetaHere)*cos(psiHere);
    RLocal[0][2] =   sin(phiHere)*sin(psiHere)     - cos(phiHere)*sin(thetaHere)*cos(psiHere);
    RLocal[1][0] =  -cos(thetaHere)*sin(psiHere);
    RLocal[1][1] =   cos(phiHere)*cos(psiHere)     - sin(phiHere)*sin(thetaHere)*sin(psiHere);
    RLocal[1][2] =   sin(phiHere)*cos(psiHere)     + cos(phiHere)*sin(thetaHere)*sin(psiHere);
    RLocal[2][0] =   sin(thetaHere);
    RLocal[2][1] =  -sin(phiHere)*cos(thetaHere);
    RLocal[2][2] =   cos(phiHere)*cos(thetaHere);
    
    
	// R Global
	RGlobal[0][0] = (*(e1In+0))* ((*(e1In+0))* RLocal[0][0] + (*(e2In+0))* RLocal[1][0] + RLocal[2][0]* (*(tIn+0)))
                  + (*(e2In+0))* ((*(e1In+0))* RLocal[0][1] + (*(e2In+0))* RLocal[1][1] + RLocal[2][1]* (*(tIn+0)))
                  + (*(tIn+0))* ((*(e1In+0))* RLocal[0][2] + (*(e2In+0))* RLocal[1][2] + RLocal[2][2]* (*(tIn+0)));
	RGlobal[0][1] = (*(e1In+1))* ((*(e1In+0))* RLocal[0][0] + (*(e2In+0))* RLocal[1][0] + RLocal[2][0]* (*(tIn+0)))
                  + (*(e2In+1))* ((*(e1In+0))* RLocal[0][1] + (*(e2In+0))* RLocal[1][1] + RLocal[2][1]* (*(tIn+0)))
                  + (*(tIn+1))* ((*(e1In+0))* RLocal[0][2] + (*(e2In+0))* RLocal[1][2] + RLocal[2][2]* (*(tIn+0)));
	RGlobal[0][2] = (*(e1In+2))* ((*(e1In+0))* RLocal[0][0] + (*(e2In+0))* RLocal[1][0] + RLocal[2][0]* (*(tIn+0)))
                  + (*(e2In+2))* ((*(e1In+0))* RLocal[0][1] + (*(e2In+0))* RLocal[1][1] + RLocal[2][1]* (*(tIn+0)))
                  + (*(tIn+2))* ((*(e1In+0))* RLocal[0][2] + (*(e2In+0))* RLocal[1][2] + RLocal[2][2]* (*(tIn+0)));
	RGlobal[1][0] = (*(e1In+0))* ((*(e1In+1))* RLocal[0][0] + (*(e2In+1))* RLocal[1][0] + RLocal[2][0]* (*(tIn+1)))
                  + (*(e2In+0))* ((*(e1In+1))* RLocal[0][1] + (*(e2In+1))* RLocal[1][1] + RLocal[2][1]* (*(tIn+1)))
                  + (*(tIn+0))* ((*(e1In+1))* RLocal[0][2] + (*(e2In+1))* RLocal[1][2] + RLocal[2][2]* (*(tIn+1)));
	RGlobal[1][1] = (*(e1In+1))* ((*(e1In+1))* RLocal[0][0] + (*(e2In+1))* RLocal[1][0] + RLocal[2][0]* (*(tIn+1)))
                  + (*(e2In+1))* ((*(e1In+1))* RLocal[0][1] + (*(e2In+1))* RLocal[1][1] + RLocal[2][1]* (*(tIn+1)))
                  + (*(tIn+1))* ((*(e1In+1))* RLocal[0][2] + (*(e2In+1))* RLocal[1][2] + RLocal[2][2]* (*(tIn+1)));
	RGlobal[1][2] = (*(e1In+2))* ((*(e1In+1))* RLocal[0][0] + (*(e2In+1))* RLocal[1][0] + RLocal[2][0]* (*(tIn+1)))
                  + (*(e2In+2))* ((*(e1In+1))* RLocal[0][1] + (*(e2In+1))* RLocal[1][1] + RLocal[2][1]* (*(tIn+1)))
                  + (*(tIn+2))* ((*(e1In+1))* RLocal[0][2] + (*(e2In+1))* RLocal[1][2] + RLocal[2][2]* (*(tIn+1)));
	RGlobal[2][0] = (*(e1In+0))* ((*(e1In+2))* RLocal[0][0] + (*(e2In+2))* RLocal[1][0] + RLocal[2][0]* (*(tIn+2)))
                  + (*(e2In+0))* ((*(e1In+2))* RLocal[0][1] + (*(e2In+2))* RLocal[1][1] + RLocal[2][1]* (*(tIn+2)))
                  + (*(tIn+0))* ((*(e1In+2))* RLocal[0][2] + (*(e2In+2))* RLocal[1][2] + RLocal[2][2]* (*(tIn+2)));
	RGlobal[2][1] = (*(e1In+1))* ((*(e1In+2))* RLocal[0][0] + (*(e2In+2))* RLocal[1][0] + RLocal[2][0]* (*(tIn+2)))
                  + (*(e2In+1))* ((*(e1In+2))* RLocal[0][1] + (*(e2In+2))* RLocal[1][1] + RLocal[2][1]* (*(tIn+2)))
                  + (*(tIn+1))* ((*(e1In+2))* RLocal[0][2] + (*(e2In+2))* RLocal[1][2] + RLocal[2][2]* (*(tIn+2)));
	RGlobal[2][2] = (*(e1In+2))* ((*(e1In+2))* RLocal[0][0] + (*(e2In+2))* RLocal[1][0] + RLocal[2][0]* (*(tIn+2)))
                  + (*(e2In+2))* ((*(e1In+2))* RLocal[0][1] + (*(e2In+2))* RLocal[1][1] + RLocal[2][1]* (*(tIn+2)))
                  + (*(tIn+2))* ((*(e1In+2))* RLocal[0][2] + (*(e2In+2))* RLocal[1][2] + RLocal[2][2]* (*(tIn+2)));
	
	// New unit vectors
	*(tOut+0) = RGlobal[0][0]*(*(tIn+0)) + RGlobal[0][1]*(*(tIn+1)) + RGlobal[0][2]*(*(tIn+2));
	*(tOut+1) = RGlobal[1][0]*(*(tIn+0)) + RGlobal[1][1]*(*(tIn+1)) + RGlobal[1][2]*(*(tIn+2));
	*(tOut+2) = RGlobal[2][0]*(*(tIn+0)) + RGlobal[2][1]*(*(tIn+1)) + RGlobal[2][2]*(*(tIn+2));	
	
	*(e1Out+0) = RGlobal[0][0]*(*(e1In+0)) + RGlobal[0][1]*(*(e1In+1)) + RGlobal[0][2]*(*(e1In+2));
	*(e1Out+1) = RGlobal[1][0]*(*(e1In+0)) + RGlobal[1][1]*(*(e1In+1)) + RGlobal[1][2]*(*(e1In+2));
	*(e1Out+2) = RGlobal[2][0]*(*(e1In+0)) + RGlobal[2][1]*(*(e1In+1)) + RGlobal[2][2]*(*(e1In+2));	
	
	*(e2Out+0) = RGlobal[0][0]*(*(e2In+0)) + RGlobal[0][1]*(*(e2In+1)) + RGlobal[0][2]*(*(e2In+2));
	*(e2Out+1) = RGlobal[1][0]*(*(e2In+0)) + RGlobal[1][1]*(*(e2In+1)) + RGlobal[1][2]*(*(e2In+2));
	*(e2Out+2) = RGlobal[2][0]*(*(e2In+0)) + RGlobal[2][1]*(*(e2In+1)) + RGlobal[2][2]*(*(e2In+2));
    

    // Re-normalize and re-orthogonalize unit vectors (using Modified Gram Schmidt algorithm)
    norm = sqrt((*(tOut+0))*(*(tOut+0)) + (*(tOut+1))*(*(tOut+1)) + (*(tOut+2))*(*(tOut+2)));
    *(tOut+0) = *(tOut+0)/norm;
    *(tOut+1) = *(tOut+1)/norm;
    *(tOut+2) = *(tOut+2)/norm;
        
    e1_dot_t = (*(tOut+0))*(*(e1Out+0))+(*(tOut+1))*(*(e1Out+1))+(*(tOut+2))*(*(e1Out+2));
    
    *(e1Out+0) = *(e1Out+0) - e1_dot_t*(*(tOut+0));
    *(e1Out+1) = *(e1Out+1) - e1_dot_t*(*(tOut+1));
    *(e1Out+2) = *(e1Out+2) - e1_dot_t*(*(tOut+2));
    
    norm = sqrt((*(e1Out+0))*(*(e1Out+0)) + (*(e1Out+1))*(*(e1Out+1)) + (*(e1Out+2))*(*(e1Out+2)));
    *(e1Out+0) = *(e1Out+0)/norm;
    *(e1Out+1) = *(e1Out+1)/norm;
    *(e1Out+2) = *(e1Out+2)/norm;

    e2_dot_t  = (*(tOut+0))*(*(e2Out+0))+(*(tOut+1))*(*(e2Out+1))+(*(tOut+2))*(*(e2Out+2));
    e2_dot_e1 =(*(e1Out+0))*(*(e2Out+0))+(*(e1Out+1))*(*(e2Out+1))+(*(e1Out+2))*(*(e2Out+2));
    
    *(e2Out+0) = *(e2Out+0) - e2_dot_t*(*(tOut+0)) - e2_dot_e1*(*(e1Out+0));
    *(e2Out+1) = *(e2Out+1) - e2_dot_t*(*(tOut+1)) - e2_dot_e1*(*(e1Out+1));
    *(e2Out+2) = *(e2Out+2) - e2_dot_t*(*(tOut+2)) - e2_dot_e1*(*(e1Out+2));
    
    norm = sqrt((*(e2Out+0))*(*(e2Out+0)) + (*(e2Out+1))*(*(e2Out+1)) + (*(e2Out+2))*(*(e2Out+2)));
    *(e2Out+0) = *(e2Out+0)/norm;
    *(e2Out+1) = *(e2Out+1)/norm;
    *(e2Out+2) = *(e2Out+2)/norm;
   
    if (0) // print stuff out for debugging
    {
        // PRINT RGlobal
        printf("At nt=%ld, i=%ld\n", nt, i);
        printf(" Rotation matrix:\n");
        printf(" %f,", RGlobal[0][0]);
        printf(" %f,", RGlobal[0][1]);
        printf(" %f,", RGlobal[0][2]);
        printf(" \n");
        printf(" %f,", RGlobal[1][0]);
        printf(" %f,", RGlobal[1][1]);
        printf(" %f,", RGlobal[1][1]);
        printf(" \n");
        printf(" %f,", RGlobal[2][0]);
        printf(" %f,", RGlobal[2][1]);
        printf(" %f,", RGlobal[2][2]);
        printf(" \n");
        
        // PRINT RTR (should be the identity matrix)
        printf(" RTR:\n");
        
        printf(" %f,", RGlobal[0][0]*RGlobal[0][0]+RGlobal[1][0]*RGlobal[1][0]+RGlobal[2][0]*RGlobal[2][0]);
        printf(" %f,", RGlobal[0][0]*RGlobal[0][1]+RGlobal[1][0]*RGlobal[1][1]+RGlobal[2][0]*RGlobal[2][1]);
        printf(" %f,", RGlobal[0][0]*RGlobal[0][2]+RGlobal[1][0]*RGlobal[1][2]+RGlobal[2][0]*RGlobal[2][2]);
        printf(" \n");
        printf(" %f,", RGlobal[0][1]*RGlobal[0][0]+RGlobal[1][1]*RGlobal[1][0]+RGlobal[2][1]*RGlobal[2][0]);
        printf(" %f,", RGlobal[0][1]*RGlobal[0][1]+RGlobal[1][1]*RGlobal[1][1]+RGlobal[2][1]*RGlobal[2][1]);
        printf(" %f,", RGlobal[0][1]*RGlobal[0][2]+RGlobal[1][1]*RGlobal[1][2]+RGlobal[2][1]*RGlobal[2][2]);
        printf(" \n");
        printf(" %f,", RGlobal[0][2]*RGlobal[0][0]+RGlobal[1][2]*RGlobal[1][0]+RGlobal[2][2]*RGlobal[2][0]);
        printf(" %f,", RGlobal[0][2]*RGlobal[0][1]+RGlobal[1][2]*RGlobal[1][1]+RGlobal[2][2]*RGlobal[2][1]);
        printf(" %f,", RGlobal[0][2]*RGlobal[0][2]+RGlobal[1][2]*RGlobal[1][2]+RGlobal[2][2]*RGlobal[2][2]);
        printf(" \n");
        
        // PRINT dot products of Cosserat vectors (should have zero dot products)
        printf("t dot e1 = %lf, \ne1 dot e2 = %lf\n",
               (*(tOut+0))*(*(e1Out+0))+(*(tOut+1))*(*(e1Out+1))+(*(tOut+2))*(*(e1Out+2)),
               (*(e2Out+0))*(*(e1Out+0))+(*(e2Out+1))*(*(e1Out+1))+(*(e2Out+2))*(*(e1Out+2)));
        
        printf(" \n");
        printf(" \n");

    }
    
	
	return;
	
}

