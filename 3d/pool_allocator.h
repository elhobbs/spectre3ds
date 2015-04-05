#ifndef POOL_ALLOCATOR_H_INCLUDED_GF
#define POOL_ALLOCATOR_H_INCLUDED_GF

#include "memPool.h"


template <typename T> class linear_allocator;

template <> class linear_allocator<void>
{
public:
    typedef void* pointer;
    typedef const void* const_pointer;
    // reference to void members are impossible.
    typedef void value_type;
    template <class U> 
        struct rebind { typedef linear_allocator<U> other; };
};    

namespace pool_alloc{
    inline void destruct(char *){}
    inline void destruct(wchar_t*){}
    template <typename T> 
        inline void destruct(T *t){t->~T();}
} // namespace
    
template <typename T>
class linear_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template <class U> 
        struct rebind { typedef linear_allocator<U> other; };
    linear_allocator(){}
    pointer address(reference x) const {return &x;}
    const_pointer address(const_reference x) const {return &x;}
    pointer allocate(size_type size, linear_allocator<void>::const_pointer hint = 0)
    {
        return static_cast<pointer>(linear.alloc(size*sizeof(T)));
    }
    //for Dinkumware:
    char *_Charalloc(size_type n){return static_cast<char*>(linear.alloc(n));}
    // end Dinkumware

    template <class U> linear_allocator(const linear_allocator<U>&){}
    void deallocate(pointer p, size_type n)
    {
        //linear.dealloc(p, n);
    }
    void deallocate(void *p, size_type n)
    {
        //mem_.deallocate(p, n);
    }
    size_type max_size() const throw() {return size_t(-1) / sizeof(value_type);}
    void construct(pointer p, const T& val)
    {
        new(static_cast<void*>(p)) T(val);
    }
    void construct(pointer p)
    {
        new(static_cast<void*>(p)) T();
    }
    void destroy(pointer p){pool_alloc::destruct(p);}
    static void dump(){
		//mem_.dump();
	};
//private:
    
    //static pool mem_;
};

//template <typename T> pool linear_allocator<T>::mem_;

template <typename T, typename U>
inline bool operator==(const linear_allocator<T>&, const linear_allocator<U>){return true;}

template <typename T, typename U>
inline bool operator!=(const linear_allocator<T>&, const linear_allocator<U>){return false;}


// For VC6/STLPort 4-5-3 see /stl/_alloc.h, line 464
// "If custom allocators are being used without member template classes support :
// user (on purpose) is forced to define rebind/get operations !!!"
#ifdef _WIN32
#define POOL_ALLOC_CDECL __cdecl
#else
#define POOL_ALLOC_CDECL
#endif

namespace std{
template <class _Tp1, class _Tp2>
inline linear_allocator<_Tp2>& POOL_ALLOC_CDECL
__stl_alloc_rebind(linear_allocator<_Tp1>& __a, const _Tp2*) 
{  
    return (linear_allocator<_Tp2>&)(__a); 
}


template <class _Tp1, class _Tp2>
inline linear_allocator<_Tp2> POOL_ALLOC_CDECL
__stl_alloc_create(const linear_allocator<_Tp1>&, const _Tp2*) 
{ 
    return linear_allocator<_Tp2>(); 
}

} // namespace std
// end STLPort

#endif

