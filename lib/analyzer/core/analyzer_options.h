#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_OPTIONS_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_OPTIONS_H


namespace clsa {

    struct analyzer_options {
        const bool array_values = false;
        const int loop_unwinding_iterations_limit = 256;
        const int function_calls_depth_limit = 256;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_OPTIONS_H
