#ifndef CPU_RELAX_HPP
#define CPU_RELAX_HPP




inline static void cpu_relax() {
//        #if (COMPILER == MVCC)
//            _mm_pause();
//        #elif (COMPILER == GCC || COMPILER == LLVM)
        asm volatile("pause\n": : :"memory");
//        #endif
}


#endif // CPU_RELAX_HPP
