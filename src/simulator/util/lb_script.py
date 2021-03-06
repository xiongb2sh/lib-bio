import sys
import math
import numpy as np
import scipy.special as spec

##############################################################################
#                                                                            #
#                         NO BINOMIAL FUNCTIONS                              #
#                                                                            #
##############################################################################

def compute_phi(n, r, Sigma):
    prod = 1.0
    for i in range(r):
        prod *= (Sigma-1)*(n-i)/(i+1)
    return prod

def compute_mu0(n, r, q, D, Sigma):
    # mu_0 = mu_00 + mu_01
    prod_1 = 1.0
    prod_2 = 1.0
    for l in range(1,q+1):
        prod_1 *= (n-D-l) / l
    for l in range(1,r-2*D+1):
        prod_2 *= (Sigma-1)*(n-D-q-(l-1))/l
        
    return prod_1 * prod_2 * (n-D) / q

def compute_mu1(n, r, q, D, Sigma):
    pass


##############################################################################
#                                                                            #
#                         CONVENIENCE FUNCTIONS                              #
#                                                                            #
##############################################################################

binom_table = None

def construct_binom_table(n):
    global binom_table
    binom_table = np.ones([n+1,n+1])
    # 'external edges of Tartaglia's (Pascal) triangle
    for i in range(n+1):
        binom_table[i,i] = 1
        binom_table[i,1] = 1
    # remaining entries
    for i in range(3, n+1):
        for j in range(2, i):
            binom_table[i,j] = binom_table[i-1,j-1]+binom_table[i-1,j]

def test_binom_table(n):
    for i in range(1,n):
        for j in range(1,i+1):
            true = spec.binom(i,j)
            table = binom_table[i+1,j+1]
            diff = true - table
            if (diff != 0):
                raise ValueError("Binomial Table Test Fails")

'''Convenience function for computing binomial coefficient. Here all
optimization can be made (e.g., Tartaglia's triangle based computation)'''
def binomial(n,k):
    return binom_table[n+1,k+1]

def composition(n,k):
    return binom_table[n,k]

def init(n):
    construct_binom_table(n+1)


##############################################################################
#                                                                            #
#                     APPROXIMATE COUNTING FUNCTIONS                         #
#                                                                            #
##############################################################################

def tilde_mu(n, r, D, q, delta_e, Sigma, Simple=False):
    t_mu = binomial(n-r+D, q-delta_e)
    if (Simple):
        return t_mu
    else:
        a = float(n-D-q+delta_e)
        b = float(q-delta_e)
        return float(t_mu) * max(a,b) / float(n-D)

def compute_approx_fd(n, r, D, q, delta_s, delta_e, Sigma):
    fn = binomial(n-D,r-2*D) * pow(Sigma-1, r-2*D)
    return fn * tilde_mu(n,r,D,q,delta_e,Sigma)

def tilde_psi(n, r, D, q, Sigma):
    q_2_floor = int(math.floor(q/2.0))
    t_psi = binomial(q,q_2_floor) * binomial(2*D-2, q-2)    
    return t_psi

def compute_approx_fn(n, r, D, q, delta_s, delta_e, Sigma):
    rho = Sigma*delta_e + (Sigma-1)*(1-delta_e)
    return pow(rho,D)*tilde_psi(n, r, D, q, Sigma)



##############################################################################
#                                                                            #
#                           COUNTING FUNCTIONS                               #
#                                                                            #
##############################################################################


# Case q=0 (i.e., main diagonal path)
def compute_Q0(n, r, Sigma): # Ok
    return binomial(n, r) * pow(Sigma-1, r)

def compute_fd(n, r, D, q, delta_s, delta_e, Sigma): # Ok
    return binomial(n-D-1, q-delta_s-delta_e)*binomial(n-D-q+delta_e, r - 2*D)*pow(Sigma-1,r - 2*D)
    
def compute_fn(n, r, D, q, delta_s, delta_e, Sigma):  # Ok
    count = 0
    # no diagonal segment at the end
    if (delta_e == 0):
        d_max = min(q-1,D)
        for d in range(1,d_max+1):
            count += (binomial(q,d) * composition(D, d)
                      * composition(D,q-d))
        count *= pow(Sigma-1, D)
    # diagonal at the end
    else:
        # ending del
        d_bar = min(q-2,D-1)
        for d in range(d_bar):
            count += (binomial(q-1,d-1) * composition(D,d)
                      * composition(D, q-d-1))
        count *= pow(Sigma-1, D)
        
        # ending ins
        count += pow(Sigma, D) * composition(D, q-1)
        i_bar = min(q-2, D-1)
        for i in range(1,i_bar+1):
            for j in range(1,D-i+1):
                count += (binomial(q-1, i) * composition(D-j, i)
                          * composition(D, q-i-1)
                          * pow(Sigma, j) * pow(Sigma, D-j))
            
    return count

##############################################################################
#                                                                            #
#                            BOUND FUNCTIONS                                 #
#                                                                            #
##############################################################################

def count_canonical_annotated_path_r_D_q(n, r, D, q, Sigma):
    count = 0
    # sum of fd*fn
    for delta_s in range(2):
        for delta_e in range(2):
            fd = compute_fd(n, r, D, q, delta_s, delta_e, Sigma) # fd=mu
            fn = compute_fn(n, r, D, q, delta_s, delta_e, Sigma) # fn=psi
            count += fd*fn
    return count
            
def count_canonical_annotated_path_r_D(n, r, D, Sigma): # OK
    if (D == 0):
        print("\t{0}\t{1}\t{2}\t{3}".format(r,D,0,compute_Q0(n, r, Sigma)))
        return compute_Q0(n, r, Sigma) # Q0 + ...
    
    count = 0
    q_max = min(2*D, n - r + D +1) # +1 is delta_e
    for q in range(2,q_max+1):
        _Qq = count_canonical_annotated_path_r_D_q(n, r, D, q, Sigma)
        count += _Qq
        print("\t{0}\t{1}\t{2}\t{3}".format(r,D,q,_Qq))
    return count

'''Counts the number of canonical annotated paths of cost r: S_r^UB'''
def count_canonical_annotated_path_r(n, r, Sigma): # OK
    count = 0
    Dmax = int(math.floor(r / 2.0)) 
    for D in range(Dmax+1):
        ub_r_d = count_canonical_annotated_path_r_D(n, r, D, Sigma)
        count += ub_r_d #count_canonical_annotated_path_r_D(n, r, D, Sigma)
        #print("\t{2}\t{0}\t{1}".format(D,ub_r_d,r))
    return count;

'''Returns the lower bound for given n, the saturation radius and the
upper bounds on all the hulls between 0 and the saturation radius'''
def bound(n, Sigma=4): # OK
    hulls_bound = [(i,0) for i in range(n+1)]    
    remaining_strings = pow(Sigma,n) - 1
    lb = 0
    r = 1    
    while (remaining_strings > 0 and r <= n):
        lb += remaining_strings
        hull_count = count_canonical_annotated_path_r(n,r, Sigma)             
        remaining_strings = remaining_strings - hull_count
        hulls_bound[r] = (r,lb)
        r += 1
    
    return (lb, hulls_bound, r-1)


##############################################################################
#                                                                            #
#                                 TEST                                       #
#                                                                            #
##############################################################################

def run_tests():
    n = 8
    Sigma = 4
    construct_binom_table(n+1)
    for r in range(n+1):
        phi = compute_phi(n, r, Sigma)
        Q0 = compute_Q0(n,r,Sigma)
        #print(phi-Q0)
        D_max = int(math.floor(r/2.0))
        for D in range(D_max+1):
            bar_q = min(2*D, n-r+D+1)
            for q in range(2,bar_q+1):
                fd_00 = compute_fd(n, r, D, q, 0, 0, Sigma)
                fd_10 = compute_fd(n, r, D, q, 1, 0, Sigma)
                mu_0 = compute_mu0(n, r, q, D, Sigma)
                diff = mu_0 - (fd_00+fd_10)
                print("{0} - ({6}+{7}) = {2}\t {3},{4},{5}".format(mu_0 , (fd_00+fd_10), diff,r,q,D,fd_00,fd_10))
    # tests tilde_mu
    print("\n**** TESTS FOR tilde_mu ****\n")
    print("(n, r, D, q)")
    print("----------------------")
    for r in range(n+1):
        D_max = int(math.floor(r/2.0))
        for D in range(D_max+1):
            bar_q = min(2*D, n-r+D+1)
            for q in range(2,bar_q+1):                
                t_mu = tilde_mu(n, r, D, q, 0, Sigma)
                t_mu_s = tilde_mu(n, r, D, q, 0, Sigma, True)
                fd_appr = compute_approx_fd(n,r,D,q,0,0,Sigma)
                print("({0}, {1}, {2}, {3}) ---> {4:.1f}\t{5}\t{6:.1f}".format(n,r,D,q,t_mu,t_mu_s,fd_appr))

    print("\n**** TESTS FOR tilde_psi ****\n")
    print("(n, r, D, q)")
    print("----------------------")
    for r in range(n+1):
        D_max = int(math.floor(r/2.0))
        for D in range(D_max+1):
            bar_q = min(2*D, n-r+D+1)
            for q in range(2,bar_q+1):                
                t_psi = tilde_psi(n, r, D, q, Sigma)
                fn_appr = compute_approx_fn(n, r, D, q, 0, 1, Sigma)
                print("({0}, {1}, {2}, {3}) ---> {4:.0f}\t{5:.0f}".format(n,r,D,q,t_psi,fn_appr))

        

##############################################################################
#                                                                            #
#                                 MAIN                                       #
#                                                                            #
##############################################################################

if __name__ == "__main__":
    if (sys.argv.count("--help") > 0 or sys.argv.count("-h") > 0):
        print("\nUSAGE\n\tpython lb_script.py [n_max] [verbosity] [options]\n\n")
        print("  Options\n\t-h, --help\n")
        exit(0)

    if (sys.argv.count("--test") > 0):
        run_tests()
        exit(0)
    
    n_max = 16
    if (len(sys.argv) > 1):
        n_max = int(sys.argv[1])

    verbosity = 0
    if (len(sys.argv) > 2):
        verbosity = int(sys.argv[2])

    n_min = 1
    if (len(sys.argv) > 3):
        n_min = int(sys.argv[3])
        
    construct_binom_table(n_max+1)
#    test_binom_table(n_max)

    for n in range(n_min,n_max+1):
        norm_const = float(n * pow(4,n))
        (lb, hulls_bound, r_sat) = bound(n)
        if (verbosity > 0):
            print("  r\tS_r\n----------------")
            for entry in hulls_bound:
                print("  {0}\t{1}".format(entry[0],entry[1]))
        print("{0}\t{1: .12f}\t{2}".format(n, lb / norm_const, r_sat))
