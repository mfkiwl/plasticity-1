//tension BVP
//general headers
#include <fstream>
#include <sstream>
using namespace std;


#define feOrder   1
#define quadOrder 2 
#define meshRefineFactor 3
#define writeOutput true
#define linearSolverType PETScWrappers::SolverCG
#define totalNumIncrements 100
#define maxLinearSolverIterations 5000
#define relLinearSolverTolerance  1.0e-10
#define maxNonLinearIterations 30
#define absNonLinearTolerance 1.0e-18
#define relNonLinearTolerance 1.0e-10
#define stopOnConvergenceFailure false

//Read json input 
#include "../../../utils/json/json_spirit.h"
#include "../../../utils/json/json_spirit_reader_template.h"

//dealIIheaders
#include "../../../src/materialModels/crystalPlasticity/crystalPlasticity.cc"
 
//Mark boundaries for applying Dirichlet BC's
template <int dim>
void crystalPlasticity<dim>::markBoundaries(){
  typename DoFHandler<dim>::active_cell_iterator 
    cell = this->dofHandler.begin_active(), 
    endc = this->dofHandler.end();

  //All boundaries are by marked with flag '0' by default. 
  //To pick specific boundaries, one needs to mark them 
  //with integer flags and use those flags in apply_dirichlet_conditons()
  for (;cell!=endc; ++cell){
    if (cell->is_locally_owned()){
      for (unsigned int f=0; f<GeometryInfo<dim>::faces_per_cell; ++f){
				if (cell->face(f)->at_boundary()){
					const Point<dim> face_center = cell->face(f)->center();
					if (face_center[0] == 0.0){
						cell->face(f)->set_boundary_indicator (1); //back boundary
					}
					else if(face_center[0] == 1.0){
						cell->face(f)->set_boundary_indicator (2); //front boundary
					}
					else if(face_center[1] == 0.0){
						cell->face(f)->set_boundary_indicator (3); //left boundary
					}
					else if(face_center[2] == 0.0){
						cell->face(f)->set_boundary_indicator (4); //bottom boundary
					}
				}
      }
    }
  }
}


//Class to set Dirichlet BC values 
template <int dim>
class BCFunction : public Function<dim>{
  public:
  BCFunction(): Function<dim> (dim){}
  void vector_value (const Point<dim>   &p, Vector<double>   &values) const{
    Assert (values.size() == dim, ExcDimensionMismatch (values.size(), dim));    
    values[0]=0.0001; // displacement along X-Direction
  }
};

//Apply Dirchlet BCs for simple tension BVP
template <int dim>
void crystalPlasticity<dim>::applyDirichletBCs(){
  this->constraints.clear ();
  this->constraints.reinit (this->locally_relevant_dofs);
  DoFTools::make_hanging_node_constraints (this->dofHandler, this->constraints);
	std::vector<bool> mechanicsBoundary_Z1 (dim, false); mechanicsBoundary_Z1[0]=true;
	std::vector<bool> mechanicsBoundary_Z2 (dim, false); mechanicsBoundary_Z2[0]=true;
	std::vector<bool> mechanicsBoundary_Z3 (dim, false); mechanicsBoundary_Z3[1]=true;
	std::vector<bool> mechanicsBoundary_Z4 (dim, false); mechanicsBoundary_Z4[2]=true;
  //u1 applied on X1=1
	if (this->currentIteration==0) {
		VectorTools:: interpolate_boundary_values (this->dofHandler,
								2, 
								BCFunction<dim>(), 
								this->constraints,
								mechanicsBoundary_Z2);
	}
	else {
		VectorTools:: interpolate_boundary_values (this->dofHandler,
								2, 
								ZeroFunction<dim>(dim),
								this->constraints,
								mechanicsBoundary_Z2);
	}
	//u1=0 on X1=0
	VectorTools:: interpolate_boundary_values (this->dofHandler, 
								1, 
								ZeroFunction<dim>(dim), 
								this->constraints, 
								mechanicsBoundary_Z1);
	//u2=0 on X2=0
	VectorTools:: interpolate_boundary_values (this->dofHandler, 
								3, 
								ZeroFunction<dim>(dim), 
								this->constraints, 
								mechanicsBoundary_Z3);
	//u3=0 on X3=0
	VectorTools:: interpolate_boundary_values (this->dofHandler, 
								4, 
								ZeroFunction<dim>(dim), 
								this->constraints, 
								mechanicsBoundary_Z4);

  this->constraints.close ();
}

//main
int main (int argc, char **argv)
{
  Utilities::System::MPI_InitFinalize mpi_initialization(argc, argv, 1);
  try
    {
      deallog.depth_console(0);
      crystalPlasticity<3> problem;
			
    FullMatrix<double>  rot, m_alpha,n_alpha;
	const unsigned int n_slip_systems=12; 
	//const unsigned int n_orientations=triangulation.n_active_cells()*quadrature_formula.size();
	
	const unsigned int n_orientations=2^6*8;
		
	
	rot.reinit(n_orientations,3);
	n_alpha.reinit(n_slip_systems,3);
	m_alpha.reinit(n_slip_systems,3);
	string line;
	ifstream myfile;


	myfile.open("orientations");
	  int i=0;
	  int j=0;
	  if (myfile.is_open()){
		  for (unsigned int k=0; k<3*n_orientations; k++){
			  if(j==2){
				  getline(myfile,line,'\n');
			  }
			  else
				getline(myfile,line,'\t');
				rot[i][j] = atof(line.c_str()); 
				//pcout<<line.c_str()<<'\t';
				j++;
				if(j==3){
				i=i+1;
				j=0;
				//pcout<<'\n';
			}
			
	   }
	 myfile.close();
	  }
	  else cout << "Unable to open orientations file"; 
	  myfile.clear() ;
	  myfile.open("slipNormals");
	  i=0;
	  j=0;
	  if (myfile.is_open()){
		for (unsigned int k=0; k<3*n_slip_systems; k++){	     
			getline (myfile,line,'\t');
			n_alpha[i][j] = atof(line.c_str()); 
			//pcout<<line.c_str()<<'\n';  
			j++;
				if(j==3){
					i=i+1;
					j=0;
				}
		}
		myfile.close();
	  }
	  else cout << "Unable to open slipNormals file"; 
	  myfile.clear() ;
  	  myfile.open("slipDirections");
	  i=0;
	  j=0;
	  if (myfile.is_open()){
		for (unsigned int k=0; k<3*n_slip_systems; k++){	     
			getline (myfile,line,'\t');
			m_alpha[i][j] = atof(line.c_str()); 
			//pcout<<m_alpha[i][j]<<'\n';
			j++;
				if(j==3){
					i=i+1;
					j=0;
				}
		}
		myfile.close();
	  }
	  else cout << "Unable to open slipDirections file"; 
	  myfile.clear() ;
	problem.properties.n_slip_systems=n_slip_systems;
	problem.properties.n_orientations=n_orientations;
	problem.properties.q1=1.4;
	problem.properties.q2=1.0;
	problem.properties.a=2.25;
	problem.properties.h0=180;
	problem.properties.s_s=148;
	problem.properties.s0=16;
	problem.properties.C11=170e3;
	problem.properties.C12=124e3;	
	problem.properties.rot=rot;
	problem.properties.m_alpha=m_alpha;
	problem.properties.n_alpha=n_alpha;
		

      problem.run ();
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Exception on processing: " << std::endl
                << exc.what() << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Unknown exception!" << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }
  
  return 0;
}

