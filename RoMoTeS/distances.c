/***** mx_romotes: distances.c *****/
/* Version 1.0				     */

/*Edited by ZHONG Bingzhuo, TUM, Germany, for RoMoTeS*/

/*Following functions in this file are developed bY ZHONG Bingzhuo
"SignedDistInterval"*/

/*Following functions in this file is taken from S-TaLiRo Software Written by Georgios Fainekos ASU, U.S.A.
"isPointInConvSet"
"inner_prod"
"norm"
"vec_add"
"vec_scl" */


#include <math.h>
#include "mex.h"
#include "matrix.h"
#include "distances.h"
#include "stl2tree.h"
#include "param.h"

#define MIN_DIM 100

/* computing signed distance interval. MATLAB function would be called when the dimensions of signal is bigger than one */
Deviation SignedDistInterval(Deviation *mondata_pre, ConvSet *SS, int dim)
{/*Developed on 17.09.2018, by ZHONG Bingzhuo */
	double dist;
	int ii, jj,i,j;

	/*supplementary variables for distinguishing an observation map with respect to*/
	/*only one dimensional sub space in a high dimensional output space*/
	int odcon=0;/*when this is a one dimensional constraints expressed in high dimensional form, this will be 1,other wise 0*/
	int odconp=-1;
	int search_chk=1;
	double A[2],b[2];
	int num_elment=0;
	bool u_b=false;     /*Indicating whether the upper bound lies in the permissive space*/
	bool l_b=false;     /*Indicating whether the lower bound lies in the permissive space*/

	/* Temporary vectors */
	double x0[MIN_DIM];
	double *x0d; /* In case dim is larger than MIN_DIM */
	double *Atemp, *btemp; /* temporary pointers to set the values of the mxArrays */
	Deviation *dev_temp;
	/* Temporary scalars */
	double aa, cc;
	/* Temporary mxArrays to pass to Matlab */
	mxArray *Xout, *Aout, *bout;
	mxArray *lhs[2], *rhs[3];
	const char *fields[] = {"upperb", "lowerb"};
	double *u_xx,*l_xx;
	double u_dist,l_dist;
    double mid_pre;  /*for middle point of the predicate interval*/
	Deviation cal_result;

    /*To distinguish an observation map with respect to only one dimensional sub space in a high dimensional output space*/
    if(dim>1)
    {
        for(i=0; i<SS->ncon; i++)
        {
            for (j=0; j<dim; j++)
            {
                if(SS->A[i][j]!=0)
                {
                    if(odconp==-1)
                    {/*the first time when a non-zero element is found*/
                        odconp=j;       /*record the current dimension*/
                        A[0]=SS->A[i][j];
                        b[0]=SS->b[i];
                        num_elment++;
                        odcon=1;
                    }
                    else if (odconp==j&&num_elment==1&&search_chk==1)
                    {   /*if the second non-zero element is found, it should be related to the same dimension as record in odconp*/
                        /*And array A and b should not be full, and it is still necessary to check*/
                        A[1]=SS->A[i][j];
                        b[1]=SS->b[i];
                        num_elment++;
                        odcon=1;
                    }
                    else
                    {
                        search_chk=0;
                        odcon=0;
                    }
                }
            }
        }
    }
    /*Calculating Robustness Estimate Interval*/
	if (SS->isSetRn)
    {
        cal_result.upperb=mxGetInf();
        cal_result.lowerb=mxGetInf();
        return (cal_result);
    }
	else
	{
		if (dim==1)     /*WHEN dimension is one, the distance can be directly calculated*/
		{
			u_xx=(double *)emalloc(sizeof(double));
			l_xx=(double *)emalloc(sizeof(double));
			u_xx[0]=mondata_pre[0].upperb;
			l_xx[0]=mondata_pre[0].lowerb;

			/*compute distance from upper bound of the deviation interval to the observation map*/
			dist = fabs(SS->b[0]/SS->A[0][0]-u_xx[0]);
			if (SS->ncon==2)
				dist = dmin(dist,fabs(SS->b[1]/SS->A[1][0]-u_xx[0]));
			if (isPointInConvSet(u_xx, SS, dim))
				u_dist=dist;
			else
				u_dist=-dist;

            /*if currently upper bound is equal to lower bound, lower bound does not need to be calculate*/
            if (u_xx[0]==l_xx[0])
            {
                cal_result.upperb=u_dist;
                cal_result.lowerb=u_dist;
                return(cal_result);
            }

			/*compute distance from lower bound of the deviation interval to the observation map*/
			dist = fabs(SS->b[0]/SS->A[0][0]-l_xx[0]);
			if (SS->ncon==2)
				dist = dmin(dist,fabs(SS->b[1]/SS->A[1][0]-l_xx[0]));
			if (isPointInConvSet(l_xx, SS, dim))
				l_dist=dist;
			else
				l_dist=-dist;

            /*compare u_dist and l_dist to get the correct upper and lower bound*/
            if (u_dist>l_dist)
            {
                cal_result.upperb=u_dist;
                cal_result.lowerb=l_dist;
            }
            else
            {
                cal_result.upperb=l_dist;
                cal_result.lowerb=u_dist;
            }
            /*debug_20201112: a special case. Start*/
            if (SS->ncon==2)
            {       
                mid_pre=(SS->b[0]/SS->A[0][0]+SS->b[1]/SS->A[1][0])/2;
                if((SS->A[0][0]*mid_pre<SS->b[0])&&(SS->A[1][0]*mid_pre<SS->b[1])&&(mid_pre>l_xx[0])&&(mid_pre<u_xx[0]))
                {
                    cal_result.upperb=fabs(mid_pre-SS->b[0]/SS->A[0][0]);
                }
            }
            /*debug_20201112: a special case. End*/
            return(cal_result);
		}
		else if(odcon==1)
        {/*There is an observation map with respect to only one dimensional sub space in a high dimensional output space*/
            /*-------------------------------*/
            u_xx=(double *)emalloc(sizeof(double));
			l_xx=(double *)emalloc(sizeof(double));
			u_xx[0]=mondata_pre[odconp].upperb;
			l_xx[0]=mondata_pre[odconp].lowerb;

			/*compute distance from upper bound of the deviation interval to the observation map*/
			dist = fabs(b[0]/A[0]-u_xx[0]);
			if (num_elment==2)
            {
               dist = dmin(dist,fabs(b[1]/A[1]-u_xx[0]));
               /*debug_20201112*/
               //u_b= (A[0]*u_xx[0]>b[0])&&(A[1]*u_xx[0]>b[1]);
               u_b= (A[0]*u_xx[0]<b[0])&&(A[1]*u_xx[0]<b[1]);
            }
            else
            {
                /*debug_20201112*/
                //u_b= (A[0]*u_xx[0]>b[0]);
                u_b= (A[0]*u_xx[0]<b[0]);
            }

			if (u_b)
				u_dist=dist;
			else
				u_dist=-dist;

            /*if currently upper bound is equal to lower bound, lower bound does not need to be calculate*/
            if (u_xx[0]==l_xx[0])
            {
                cal_result.upperb=u_dist;
                cal_result.lowerb=u_dist;
                return(cal_result);
            }

			/*compute distance from lower bound of the deviation interval to the observation map*/
			dist = fabs(b[0]/A[0]-l_xx[0]);
			if (num_elment==2)
            {
                dist = dmin(dist,fabs(b[1]/A[1]-l_xx[0]));
                /*debug_20201112*/
                //l_b= (A[0]*l_xx[0]>b[0])&&(A[1]*l_xx[0]>b[1]);
                l_b= (A[0]*l_xx[0]<b[0])&&(A[1]*l_xx[0]<b[1]);
            }
			else
            {
                /*debug_20201112*/
                //l_b= (A[0]*l_xx[0]>b[0]);
                l_b= (A[0]*l_xx[0]<b[0]);
            }

			if (l_b)
				l_dist=dist;
			else
				l_dist=-dist;

            /*compare u_dist and l_dist to get the correct upper and lower bound*/
            if (u_dist>l_dist)
            {
                cal_result.upperb=u_dist;
                cal_result.lowerb=l_dist;
            }
            else
            {
                cal_result.upperb=l_dist;
                cal_result.lowerb=u_dist;
            }
            
            /*debug_20201112: a special case. Start*/
            if (num_elment==2)
            {       
                mid_pre=(b[0]/A[0]+b[1]/A[1])/2;
                if((A[0]*mid_pre<b[0])&&(A[1]*mid_pre<b[1])&&(mid_pre>l_xx[0])&&(mid_pre<u_xx[0]))
                {
                    cal_result.upperb=fabs(mid_pre-b[0]/A[0]);
                }
            }
            /*debug_20201112: a special case. End*/
            return(cal_result);
            /*-----------------------------*/
        }
		else
		{
				/* Prepare data and call SignedDisInterval.m */
				/* From help: If unsuccessful in a MEX-file, the MEX-file terminates and control returns to the MATLAB prompt. */
				/* Alternatively use mexCallMATLABWithTrap */
				/*Xout = mxCreateDoubleMatrix(1, 1, mxREAL);*/
				Aout = mxCreateDoubleMatrix(SS->ncon, dim, mxREAL);
				bout = mxCreateDoubleMatrix(SS->ncon, 1, mxREAL);

                Xout=mxCreateStructMatrix(1, dim, 2, fields);
				Atemp = mxGetPr(Aout);
				btemp = mxGetPr(bout);

				/*read the information of deviation*/
                for(ii=0; ii<dim; ii++)
                {
                    mxSetField(Xout, ii, "upperb",  mxCreateDoubleScalar(mondata_pre[ii].upperb));
                    mxSetField(Xout, ii, "lowerb",  mxCreateDoubleScalar(mondata_pre[ii].lowerb));
                }


                /*read the information of map*/
				for(ii=0; ii<SS->ncon; ii++)
				{
					btemp[ii] = SS->b[ii];
					for (jj=0; jj<dim; jj++)
						Atemp[jj*SS->ncon+ii] = SS->A[ii][jj];
				}
				rhs[0] = Xout; rhs[1] = Aout; rhs[2] = bout;
				mexCallMATLAB(2,lhs,3,rhs,"SignedDisInterval");

                /*transfer output*/
				l_dist = *(mxGetPr(lhs[0]));
				u_dist = *(mxGetPr(lhs[1]));
				cal_result.upperb=u_dist;
                cal_result.lowerb=l_dist;

                /*Release storage space*/
				mxDestroyArray(lhs[0]);
				mxDestroyArray(lhs[1]);
				mxDestroyArray(bout);
				mxDestroyArray(Aout);
				mxDestroyArray(Xout);
				return(cal_result);
		}
	}
}


/*To judge whether a point is in a set*/
int isPointInConvSet(double *xx, ConvSet *SS, int dim)
{/*Taken from S-TaLiRo Software*/
	int i;
	for (i=0; i<SS->ncon; i++)
        if (inner_prod(SS->A[i],xx,dim)>SS->b[i])
			return(0);
	return(1);
}

/* Inner product of vectors vec1 and vec2 */
double inner_prod(double *vec1, double *vec2, int dim)
{/*Taken from S-TaLiRo Software*/
	int i;
	double sum=0.0;
	for (i=0; i<dim; i++)
		sum += vec1[i]*vec2[i];
	return(sum);
}

/* Computation of the Euclidean norm of a vector */
double norm(double *vec, int dim)
{/*Taken from S-TaLiRo Software*/
	int i;
	double nr=0.0;
	for (i=0; i<dim; i++)
		nr += vec[i]*vec[i];
	return(sqrt(nr));
}

/* Addition of vectors vec1 and vec2
   The result is returned at vec0 */
void vec_add(double* vec0, double *vec1, double *vec2, int dim)
{/*Taken from S-TaLiRo Software*/
	int i;
	for (i=0; i<dim; i++)
		vec0[i] = vec1[i]+vec2[i];
}

/* Multiplication of vector (vec1) with a scalar (scl)
   The result is returned at vec0 */
void vec_scl(double *vec0, double scl, double *vec1, int dim)
{/*Taken from S-TaLiRo Software*/
	int i;
	for (i=0; i<dim; i++)
		vec0[i] = scl*vec1[i];
}
