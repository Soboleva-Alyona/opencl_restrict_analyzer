#include "analyzer.h"

#include "clang/Tooling/Tooling.h"
#include "analyzer_parameters.h"
#include "ast_consumer.h"
#include <clang/Basic/FileManager.h>
#include <clang/Basic/Builtins.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <llvm/Support/Host.h>

clsma::analyzer::analyzer(std::string_view filename) : compiler_instance() {
    compiler_instance.createDiagnostics();

    auto target_opts = std::make_shared<clang::TargetOptions>();
    target_opts->Triple = llvm::sys::getDefaultTargetTriple();
    auto* target_info = clang::TargetInfo::CreateTargetInfo(compiler_instance.getDiagnostics(), target_opts);
    compiler_instance.setTarget(target_info);

    compiler_instance.createFileManager();
    auto& file_manager = compiler_instance.getFileManager();

    compiler_instance.createSourceManager(file_manager);
    auto& source_manager = compiler_instance.getSourceManager();

    auto& header_search_opts = compiler_instance.getHeaderSearchOpts();
    //header_search_opts.AddPath("/Library/Developer/CommandLineTools/usr/lib/clang/12.0.0/include/", clang::frontend::IncludeDirGroup::Angled, false, false);

    auto& lang_opts = compiler_instance.getLangOpts();
    lang_opts.IncludeDefaultHeader = 1;
    clang::CompilerInvocation::setLangDefaults(lang_opts, clang::InputKind(clang::Language::OpenCL),
                                               target_info->getTriple(),
                                               compiler_instance.getPreprocessorOpts().Includes,
                                               clang::LangStandard::lang_opencl20);

    file_entry = file_manager.getFile(filename).get();
    source_manager.setMainFileID(source_manager.getOrCreateFileID(file_entry, clang::SrcMgr::C_System));
}

void clsma::analyzer::analyze(uint32_t checks, std::string_view kernel_name,
                              uint32_t work_dim, const size_t* global_work_size, const size_t* local_work_size,
                              size_t args_count, const size_t* arg_sizes, void** arg_values) {
    std::vector<std::pair<size_t, void*>> args;
    args.reserve(args_count);
    for (size_t i = 0; i < args_count; ++i) {
        args.emplace_back(arg_sizes[i], arg_values[i]);
    }
    const analyzer_parameters analyzer_parameters = {
            .checks = checks,
            .kernel_name = std::string(kernel_name),
            .args = args,
            .work_dim = work_dim,
            .global_work_size = std::vector(global_work_size, global_work_size + work_dim),
            .local_work_size = std::vector(local_work_size, local_work_size + work_dim),
            .options = {
                    .array_values = false
            }
    };

    compiler_instance.createPreprocessor(clang::TU_Complete);
    compiler_instance.createASTContext();
    compiler_instance.setASTConsumer(std::make_unique<ast_consumer>(analyzer_parameters));
    compiler_instance.getDiagnosticClient().BeginSourceFile(compiler_instance.getLangOpts(),
                                                            &compiler_instance.getPreprocessor());
    clang::ParseAST(compiler_instance.getPreprocessor(), &compiler_instance.getASTConsumer(),
                    compiler_instance.getASTContext());
    compiler_instance.getDiagnosticClient().EndSourceFile();
}


