#include <armadillo>
#include <math.h>
#include <boost/math/distributions.hpp>
#include "arma/Types/VECTOR.h"
#include "arma/Types/MATRIX.h"

#define ADD(x) x += other.x;

//! [ex-logreg-desc]
/*
 *  GLA_DESC
 *      NAME(</LogisticRegressionIRLS/>)
 *      INPUTS(</(x, VECTOR), (y, DOUBLE)/>)
 *      OUTPUTS(</(coefficients, VECTOR)/>)
 *      CONSTRUCTOR(</(width, BIGINT)/>)
 *      RESULT_TYPE(</multi/>)
 *      OPT_ITERABLE
 *      LIBS(armadillo)
 *  END_DESC
 */
//! [ex-logreg-desc]

using namespace arma;

// Declaration
class LogisticRegressionIRLS;

//! [ex-logreg-const-state]
class LogisticRegressionIRLS_ConstState {
    // Inter-iteration components
    VECTOR coef;
    uint64_t iteration;

    // Constructor arguments
    BIGINT width;

    friend class LogisticRegressionIRLS;

public:
    LogisticRegressionIRLS_ConstState( const BIGINT& width ) :
        width(width),
        // Inter-iteration components
        coef            (width),
        iteration       (0)
    {
        coef.zeros();
    }

};
//! [ex-logreg-const-state]

class LogisticRegressionIRLS {
    // Inter-iteration components in const state
    const LogisticRegressionIRLS_ConstState & constState;

    // Intra-iteration components
    uint64_t numRows;           // number of rows processed this iteration
    VECTOR X_transp_Az;         // X^T A z
    MATRIX X_transp_AX;         // X^T A X
    DOUBLE loglikelihood;       // ln(l(c))

    // Keep track of internal iteration for retrieving results
    uint64_t tuplesProduced;

    // Additional members needed when calculating results.
    VECTOR diag_of_inv_X_transp_AX;     // Diagonal of the inverse of X_transp_AX

    // Helper function
    double sigma( double x ) {
        return 1.0 / (1.0 + std::exp(-x));
    }

public:

//! [ex-logreg-constructor]
    LogisticRegressionIRLS( const LogisticRegressionIRLS_ConstState & state ) :
        constState(state),
        // Intra-iteration components
        numRows         (0),
        X_transp_Az     (state.width),
        X_transp_AX     (state.width, state.width),
        loglikelihood   (0.0),
        // Additional members
        diag_of_inv_X_transp_AX     (state.width)
    {
        X_transp_Az.zeros();
        X_transp_AX.zeros();
    }
//! [ex-logreg-constructor]

//! [ex-logreg-additem]
    void AddItem( const VECTOR & x, const DOUBLE & y ) {
        const VECTOR & coef = constState.coef;

        ++numRows;

        double xc = dot(x, coef);

        double a = sigma(xc) * sigma(-xc);

        double az = xc * a + sigma(-y * xc) * y;

        X_transp_Az += x * az;
        X_transp_AX += x * trans(x) * a;

        loglikelihood -= std::log( 1. + std::exp(-y * xc) );
    }
//! [ex-logreg-additem]

//! [ex-logreg-addstate]
    void AddState( const LogisticRegressionIRLS & other ) {
        ADD(numRows);
        ADD(X_transp_Az);
        ADD(X_transp_AX);
        ADD(loglikelihood);
    }
//! [ex-logreg-addstate]

//! [ex-logreg-finalize]
    void Finalize() {
        // Set internal iterator
        tuplesProduced = 0;
    }
//! [ex-logreg-finalize]

//! [ex-logreg-should-iterate]
    bool ShouldIterate( LogisticRegressionIRLS_ConstState& modibleState ) {
        // References to the modifyable state's members so that we don't have
        // to specifically access the members all the time.
        VECTOR & coef = modibleState.coef;
        uint64_t & iteration = modibleState.iteration;

        MATRIX inverse_of_X_transp_AX = inv(X_transp_AX);

        coef = inverse_of_X_transp_AX * X_transp_Az;

        diag_of_inv_X_transp_AX = diagvec(inverse_of_X_transp_AX);

        // Note: not the actual iteration condition. Simplified for now.
        ++iteration;

        return (iteration < 5);
    }
//! [ex-logreg-should-iterate]

//! [ex-logreg-get-next-result]
    bool GetNextResult( VECTOR& coefficients ) {
        const VECTOR & coef = constState.coef;
        const uint64_t & iteration = constState.iteration;

        if( tuplesProduced < 1 ) { // fast track
            ++tuplesProduced;
            coefficients = coef;

            // Set up vectors to hold diagnostics
            VECTOR stdErr(coef.n_rows);
            VECTOR waldZStats(coef.n_rows);
            VECTOR waldPValues(coef.n_rows);
            VECTOR oddsRatios(coef.n_rows);

            for( size_t i = 0; i < coef.n_rows; ++i ) {
                stdErr(i) = std::sqrt(diag_of_inv_X_transp_AX(i));
                waldZStats(i) = coef(i) / stdErr(i);
                // Note: may need to add wrapper to the boost cdf function to modify
                // the domain, as cdf may throw a domain_error if the input value is
                // infinite instead of returning the correct mathematical result.
                waldPValues(i) = 2.0 * boost::math::cdf( boost::math::normal_distribution<>(),
                        - std::abs(waldZStats(i)));
                oddsRatios(i) = std::exp( coef(i) );
            }

            cout << endl;
            cout << "[LogisticRegressionIRLS] Output for iteration " << iteration << ":" << endl;
            cout << "Coefficients:" << endl;
            cout << coef << endl;
            cout << "Standard Error:" << endl;
            cout << stdErr << endl;
            cout << "Z Statistics:" << endl;
            cout << waldZStats << endl;
            cout << "P Values:" << endl;
            cout << waldPValues << endl;
            cout << "Odds Ratios:" << endl;
            cout << oddsRatios << endl;

            return true;
        }

        return false;
    }
//! [ex-logreg-get-next-result]
};
