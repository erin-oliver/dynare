/*1:*/

#include "sparse_tensor.h"
#include "fs_tensor.h"
#include "tl_exception.h"

#include <cmath> 

/*2:*/

void SparseTensor::insert(const IntSequence&key,int r,double c)
{
	TL_RAISE_IF(r<0||r>=nr,
		"Row number out of dimension of tensor in SparseTensor::insert");
	TL_RAISE_IF(key.size()!=dimen(),
		"Wrong length of key in SparseTensor::insert");
	TL_RAISE_IF(!std::isfinite(c),
		"Insertion of non-finite value in SparseTensor::insert");
	
	iterator first_pos= m.lower_bound(key);
	/*3:*/
	
	iterator last_pos= m.upper_bound(key);
	for(iterator it= first_pos;it!=last_pos;++it)
		if((*it).second.first==r){
			TL_RAISE("Duplicate <key, r> insertion in SparseTensor::insert");
			return;
		}
		
		/*:3*/
		;
		m.insert(first_pos,Map::value_type(key,Item(r,c)));
		if(first_nz_row> r)
			first_nz_row= r;
		if(last_nz_row<r)
			last_nz_row= r;
}

/*:2*/
;
/*4:*/

bool SparseTensor::isFinite()const
{
	bool res= true;
	const_iterator run= m.begin();
	while(res&&run!=m.end()){
		if(!std::isfinite((*run).second.second))
			res= false;
		++run;
	}
	return res;
}

/*:4*/
;
/*5:*/

double SparseTensor::getFoldIndexFillFactor()const
{
	int cnt= 0;
	const_iterator start_col= m.begin();
	while(start_col!=m.end()){
		cnt++;
		const IntSequence&key= (*start_col).first;
		start_col= m.upper_bound(key);
	}
	
	return((double)cnt)/ncols();
}

/*:5*/
;
/*6:*/

double SparseTensor::getUnfoldIndexFillFactor()const
{
	int cnt= 0;
	const_iterator start_col= m.begin();
	while(start_col!=m.end()){
		const IntSequence&key= (*start_col).first;
		Symmetry s(key);
		cnt+= Tensor::noverseq(s);
		start_col= m.upper_bound(key);
	}
	
	return((double)cnt)/ncols();
}



/*:6*/
;
/*7:*/

void SparseTensor::print()const
{
	printf("Fill: %3.2f %%\n",100*getFillFactor());
	const_iterator start_col= m.begin();
	while(start_col!=m.end()){
		const IntSequence&key= (*start_col).first;
		printf("Column: ");key.print();
		const_iterator end_col= m.upper_bound(key);
		int cnt= 1;
		for(const_iterator run= start_col;run!=end_col;++run,cnt++){
			if((cnt/7)*7==cnt)
				printf("\n");
			printf("%d(%6.2g)  ",(*run).second.first,(*run).second.second);
		}
		printf("\n");
		start_col= end_col;
	}
}



/*:7*/
;
/*8:*/

FSSparseTensor::FSSparseTensor(int d,int nvar,int r)
:SparseTensor(d,r,FFSTensor::calcMaxOffset(nvar,d)),
nv(nvar),sym(d)
{}

/*:8*/
;
/*9:*/

FSSparseTensor::FSSparseTensor(const FSSparseTensor&t)
:SparseTensor(t),
nv(t.nvar()),sym(t.sym)
{}

/*:9*/
;
/*10:*/

void FSSparseTensor::insert(const IntSequence&key,int r,double c)
{
	TL_RAISE_IF(!key.isSorted(),
		"Key is not sorted in FSSparseTensor::insert");
	TL_RAISE_IF(key[key.size()-1]>=nv||key[0]<0,
		"Wrong value of the key in FSSparseTensor::insert");
	SparseTensor::insert(key,r,c);
}

/*:10*/
;
/*11:*/

void FSSparseTensor::multColumnAndAdd(const Tensor&t,Vector&v)const
{
	/*12:*/
	
	TL_RAISE_IF(v.length()!=nrows(),
		"Wrong size of output vector in FSSparseTensor::multColumnAndAdd");
	TL_RAISE_IF(t.dimen()!=dimen(),
		"Wrong dimension of tensor in FSSparseTensor::multColumnAndAdd");
	TL_RAISE_IF(t.ncols()!=1,
		"The input tensor is not single-column in FSSparseTensor::multColumnAndAdd");
	
	
	/*:12*/
	;
	for(Tensor::index it= t.begin();it!=t.end();++it){
		int ind= *it;
		double a= t.get(ind,0);
		if(a!=0.0){
			IntSequence key(it.getCoor());
			key.sort();
			/*13:*/
			
			TL_RAISE_IF(key[0]<0||key[key.size()-1]>=nv,
				"Wrong coordinates of index in FSSparseTensor::multColumnAndAdd");
			
			/*:13*/
			;
			const_iterator first_pos= m.lower_bound(key);
			const_iterator last_pos= m.upper_bound(key);
			for(const_iterator cit= first_pos;cit!=last_pos;++cit){
				int r= (*cit).second.first;
				double c= (*cit).second.second;
				v[r]+= c*a;
			}
		}
	}
}


/*:11*/
;
/*14:*/

void FSSparseTensor::print()const
{
	printf("FS Sparse tensor: dim=%d, nv=%d, (%dx%d)\n",dim,nv,nr,nc);
	SparseTensor::print();
}

/*:14*/
;
/*15:*/

GSSparseTensor::GSSparseTensor(const FSSparseTensor&t,const IntSequence&ss,
							   const IntSequence&coor,const TensorDimens&td)
							   :SparseTensor(td.dimen(),t.nrows(),td.calcFoldMaxOffset()),
							   tdims(td)
{
	/*16:*/
	
	IntSequence s_offsets(ss.size(),0);
	for(int i= 1;i<ss.size();i++)
		s_offsets[i]= s_offsets[i-1]+ss[i-1];
	
	IntSequence lb(coor.size());
	IntSequence ub(coor.size());
	for(int i= 0;i<coor.size();i++){
		lb[i]= s_offsets[coor[i]];
		ub[i]= s_offsets[coor[i]]+ss[coor[i]]-1;
	}
	
	
	/*:16*/
	;
	
	FSSparseTensor::const_iterator lbi= t.getMap().lower_bound(lb);
	FSSparseTensor::const_iterator ubi= t.getMap().upper_bound(ub);
	for(FSSparseTensor::const_iterator run= lbi;run!=ubi;++run){
		if(lb.lessEq((*run).first)&&(*run).first.lessEq(ub)){
			IntSequence c((*run).first);
			c.add(-1,lb);
			insert(c,(*run).second.first,(*run).second.second);
		}
	}
	
}

/*:15*/
;
/*17:*/

void GSSparseTensor::insert(const IntSequence&s,int r,double c)
{
	TL_RAISE_IF(!s.less(tdims.getNVX()),
		"Wrong coordinates of index in GSSparseTensor::insert");
	SparseTensor::insert(s,r,c);
}

/*:17*/
;
/*18:*/

void GSSparseTensor::print()const
{
	printf("GS Sparse tensor: (%dx%d)\nSymmetry: ",nr,nc);
	tdims.getSym().print();
	printf("NVS: ");
	tdims.getNVS().print();
	SparseTensor::print();
}

/*:18*/
;

/*:1*/
