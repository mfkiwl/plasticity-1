//solve non linear system of equations for ellipticBVP class
#include "../../include/ellipticBVP.h"

//solve non-linear system of equations
template <int dim>
bool ellipticBVP<dim>::solveNonLinearSystem(){
  //residuals
  double relNorm=1.0, initialNorm=1.0e-16, currentNorm=0.0;
  //INDENTATION
  unsigned int extra_non_linear_iterations=0;
  //non linear iterations
  char buffer[200];
  currentIteration=0;
  while (currentIteration < userInputs.maxNonLinearIterations + extra_non_linear_iterations){
    //call updateBeforeIteration, if any
    //pcout << "debug solveNonLinearSystem 1\n";
    updateBeforeIteration();
    //pcout << "debug solveNonLinearSystem 2\n";
    //Calling assemble
    computing_timer.enter_section("assembly");
    assemble();
    computing_timer.exit_section("assembly");
    //pcout << "debug solveNonLinearSystem 3\n";
    if (!resetIncrement){
      //Calculate residual norms and check for convergence
      currentNorm=residual.l2_norm();
      initialNorm=std::max(initialNorm, currentNorm);
      relNorm=currentNorm/initialNorm;
      //print iteration information
      sprintf(buffer,
        "nonlinear iteration %3u [current residual: %8.2e, initial residual: %8.2e, relative residual: %8.2e]\n",
        currentIteration,
        currentNorm,
        initialNorm,
        relNorm);
        pcout << buffer;
        //if not converged, solveLinearSystem Ax=b
        computing_timer.enter_section("solve");
        solveLinearSystem(constraints, jacobian, residual, solution, solutionWithGhosts, solutionIncWithGhosts);
        computing_timer.exit_section("solve");
        currentIteration++;
      }
      //call updateAfterIteration, if any
      if (userInputs.enableIndentationBCs){

          if ((old_active_set_size!=active_set_size)&&(currentIteration>1))
          {
              freeze_out_iterations = userInputs.freezeActiveSetSteps;
              if (extra_non_linear_iterations > 12)
                  pcout << "active set not converging??? continuing...";
              else
              {
                extra_non_linear_iterations += userInputs.freezeActiveSetSteps + 1;
                pcout << "Allowing extra non-linear iteration for Active Set size change "<<old_active_set_size<<"-->"<<active_set_size<<"\n";
              }
          }
//          if ((currentIteration == 0)&&(userInputs.freezeActiveSetNewIteration))
//              freeze_out_iterations = 1;
          old_active_set_size=active_set_size;
      }

      updateAfterIteration();
      //pcout << "debug solveNonLinearSystem 4\n";
    }

    //check if maxNonLinearIterations reached

    //update old solution to new converged solution
    oldSolution=solution;
    return true;
  }
  #include "../../include/ellipticBVP_template_instantiations.h"
