#include "tpl_rw_t_wrapper.hpp"

#if defined(__NVCC__)
#  include <math_constants.h>
#endif

namespace libcloudphxx
{
  namespace lgrngn
  {
    using detail::tpl_rw_t_wrap;

    template <typename real_t, typename n_t>
    struct kernel_base
    {
      // pointer to kernel parameters device vector
      thrust_device::pointer<real_t> k_params;

      // number of user-defined parameters
      n_t n_user_params; 

      // largest radius for which efficiency is defined, 0 - n/a
      real_t r_max;
   
      //ctor
      kernel_base(thrust_device::pointer<real_t> k_params, n_t n_user_params = 0, real_t r_max = 0.) : 
        k_params(k_params), n_user_params(n_user_params), r_max(r_max) {}

      BOOST_GPU_ENABLED
      virtual real_t calc(const tpl_rw_t_wrap<real_t,n_t> &) const {return 0;}
    };


    //Golovin kernel
    template <typename real_t, typename n_t>
    struct kernel_golovin : kernel_base<real_t, n_t>
    {
      //ctor
      kernel_golovin(thrust_device::pointer<real_t> k_params) : kernel_base<real_t, n_t>(k_params, 1) {}

      BOOST_GPU_ENABLED
      virtual real_t calc(const tpl_rw_t_wrap<real_t,n_t> &tpl_wrap) const
      {
        enum { n_a_ix, n_b_ix, rw2_a_ix, rw2_b_ix, vt_a_ix, vt_b_ix, rd3_a_ix, rd3_b_ix };
#if !defined(__NVCC__)
        using std::abs;
        using std::pow;
        using std::max;
#endif
        real_t res =
#if !defined(__NVCC__)
        pi<real_t>()
#else
        CUDART_PI
#endif
        * 4. / 3.
        * kernel_base<real_t, n_t>::k_params[0]
        * max(
            thrust::get<n_a_ix>(tpl_wrap()),
            thrust::get<n_b_ix>(tpl_wrap())
          )
        * (
            pow(thrust::get<rw2_a_ix>(tpl_wrap()),real_t(3./2.)) +
            pow(thrust::get<rw2_b_ix>(tpl_wrap()),real_t(3./2.))
          );
        return res;
      }
    };


    //geometric kernel
    template <typename real_t, typename n_t>
    struct kernel_geometric : kernel_base<real_t, n_t>
    {
      //ctor
      kernel_geometric(thrust_device::pointer<real_t> k_params = thrust_device::pointer<real_t>(), n_t n_user_params = 0, real_t r_max = 0.) : 
        kernel_base<real_t, n_t>(k_params, n_user_params, r_max) {}

      //bilinear interpolation of efficiencies, required by dervied classes
      BOOST_GPU_ENABLED
      real_t bilinear_interpolation(real_t, real_t) const;

      BOOST_GPU_ENABLED
      virtual real_t calc(const tpl_rw_t_wrap<real_t,n_t> &tpl_wrap) const
      {
        enum { n_a_ix, n_b_ix, rw2_a_ix, rw2_b_ix, vt_a_ix, vt_b_ix, rd3_a_ix, rd3_b_ix };
#if !defined(__NVCC__)
        using std::abs;
        using std::pow;
        using std::max;
#endif
        real_t res =
#if !defined(__NVCC__)
        pi<real_t>()
#else
        CUDART_PI
#endif
        * max(
            thrust::get<n_a_ix>(tpl_wrap()),
            thrust::get<n_b_ix>(tpl_wrap())
          )
        * abs(
            thrust::get<vt_a_ix>(tpl_wrap()) -
            thrust::get<vt_b_ix>(tpl_wrap())
          )
        * pow(
            sqrt(thrust::get<rw2_a_ix>(tpl_wrap())) +
            sqrt(thrust::get<rw2_b_ix>(tpl_wrap())),
            real_t(2)
          );
        return res;
      }
    };


    template <typename real_t, typename n_t>
    struct kernel_geometric_with_efficiencies : kernel_geometric<real_t, n_t>
    {
      //ctor
      kernel_geometric_with_efficiencies(thrust_device::pointer<real_t> k_params, real_t r_max) : kernel_geometric<real_t, n_t>(k_params, 0, r_max) {}

      BOOST_GPU_ENABLED
      virtual real_t calc(const tpl_rw_t_wrap<real_t,n_t> &tpl_wrap) const
      {
        enum { n_a_ix, n_b_ix, rw2_a_ix, rw2_b_ix, vt_a_ix, vt_b_ix, rd3_a_ix, rd3_b_ix };

#if !defined(__NVCC__)
        using std::sqrt;
#endif
        real_t res = kernel_geometric<real_t, n_t>::bilinear_interpolation(
                       sqrt( thrust::get<rw2_a_ix>(tpl_wrap()))* 1e6,
                       sqrt( thrust::get<rw2_b_ix>(tpl_wrap()))* 1e6 );//*1e6, because efficiencies are defined for micrometers
/*
        real_t r1,r2;
        r1 = sqrt( thrust::get<rw2_a_ix>(tpl_wrap()))* 1e6;
        r2 = sqrt( thrust::get<rw2_b_ix>(tpl_wrap()))* 1e6;
        if(r1 >= kernel_base<real_t, n_t>::r_max) 
          r1 = kernel_base<real_t, n_t>::r_max - 1e-6; //TODO : improve this
        if(r2 >= kernel_base<real_t, n_t>::r_max) 
          r2 = kernel_base<real_t, n_t>::r_max - 1e-6; //TODO : improve this
        n_t i1 = detail::kernel_index<real_t, n_t>(r1);
        n_t i2 = detail::kernel_index<real_t, n_t>(r2);
        n_t iv = detail::kernel_vector_index<n_t>(i1, i2);
        real_t res = kernel_base<real_t, n_t>::k_params[iv]; 
*/
        res *= kernel_geometric<real_t, n_t>::calc(tpl_wrap);
        return res;
      }
    };
  };
};


                            
