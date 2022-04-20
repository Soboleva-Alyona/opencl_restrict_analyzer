#include "analyzer.h"

#include <clang/Basic/Builtins.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/Support/Host.h>

#include "../core/ast_consumer.h"
#include "../checkers/address_checker.h"
#include "../checkers/restrict_checker.h"

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
    // TODO: adjust header_search_opts?
    //header_search_opts.AddPath("/Library/Developer/CommandLineTools/usr/lib/clang/12.0.0/include/", clang::frontend::IncludeDirGroup::Angled, false, false);

    auto& lang_opts = compiler_instance.getLangOpts();
    lang_opts.IncludeDefaultHeader = 1;
    clang::CompilerInvocation::setLangDefaults(lang_opts, clang::InputKind(clang::Language::OpenCL),
        target_info->getTriple(), compiler_instance.getPreprocessorOpts().Includes, clang::LangStandard::lang_opencl20);

    file_entry = file_manager.getFile(filename).get();
    source_manager.setMainFileID(source_manager.getOrCreateFileID(file_entry, clang::SrcMgr::C_System));
}

void clsma::analyzer::analyze(std::uint32_t checks, std::string_view kernel_name, std::uint32_t work_dim,
                              const std::size_t* global_work_size, const std::size_t* local_work_size,
                              std::size_t args_count, const std::size_t* arg_sizes, void** arg_values,
                              const clsma::analyzer_options& options) {
    std::vector<std::pair<size_t, void*>> args;
    args.reserve(args_count);
    for (size_t i = 0; i < args_count; ++i) {
        args.emplace_back(arg_sizes[i], arg_values[i]);
    }
    const analyzer_parameters analyzer_parameters = {
        .kernel_name = std::string(kernel_name),
        .args = args,
        .work_dim = work_dim,
        .global_work_size = std::vector(global_work_size, global_work_size + work_dim),
        .local_work_size = local_work_size == nullptr ? std::nullopt
                                                      : std::make_optional<std::vector<size_t>>(local_work_size,
                local_work_size + work_dim),
        .options = options
    };

    compiler_instance.createPreprocessor(clang::TU_Complete);
    compiler_instance.createASTContext();
    compiler_instance.setASTConsumer(std::make_unique<ast_consumer>(analyzer_parameters,
        [checks](clsma::ast_consumer& consumer, clsma::analyzer_context& ctx) -> void {
            consumer.set_violation_handler([&ctx](const clsma::violation& violation) -> void {
                std::cerr << "violation at " << violation.location.printToString(ctx.ast.getSourceManager())
                          << ": " << violation.message;
            });
            if (checks & checks::address) {
                consumer.add_checker(std::make_unique<clsma::address_checker>(ctx));
            }
            if (checks & checks::restrict) {
                consumer.add_checker(std::make_unique<clsma::restrict_checker>(ctx));
            }
        }));
    compiler_instance.getDiagnosticClient().BeginSourceFile(compiler_instance.getLangOpts(),
        &compiler_instance.getPreprocessor());
    clang::ParseAST(compiler_instance.getPreprocessor(), &compiler_instance.getASTConsumer(),
        compiler_instance.getASTContext());
    compiler_instance.getDiagnosticClient().EndSourceFile();
}
