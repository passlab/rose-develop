#include <rosetollvm/CodeEmitter.h>

using namespace std;

CodeEmitter::CodeEmitter(Option &option_, Control &control, string input_file) : last_instruction_was_an_unconditional_branch(false),
                                                                                 option(option_)
{
    if (option.isDebugOutput()) {
        default_output = &cout;
    }
    else {
        default_output = control.addLLVMFile(input_file);
    }
    Cout = default_output;
}


/**
 * Start sending emitted code to the given buffer.
 */
void CodeEmitter::startOutputToBuffer(stringstream &buffer) {
    Cout = &buffer;
}

/**
 * Emit code from a given buffer.
 */
void CodeEmitter::flushBuffer(stringstream &buffer) {
    (*default_output) << buffer.str();
}

/**
 * Allocate a new buffer and start sending emitted code to it.
 *
 */
void CodeEmitter::startOutputToBuffer() {
    stringstream *buffer = new stringstream();
    bufferPool.push_back(buffer);
    Cout = buffer;
}

/**
 * Note that only for_increment codes are buffered. As these blocks of code cannot start with an unconditional branch
 * and are always followed by an end-loop label, we don't need to worry about how the 
 * last_instruction_was_an_unconditional_branch variable is set prior to "flushing" this code buffer. After emitting
 * this code, we can assert that last_instruction_was_an_unconditional_branch variable is false.
 */
void CodeEmitter::flushTopBuffer() {
    ROSE2LLVM_ASSERT(bufferPool.size() > 0);
    int top = bufferPool.size() - 1;
    (*default_output) << bufferPool[top] -> str();
    delete bufferPool[top];       // clear out content of stream
    bufferPool.resize(top);       // remove it from tuple
    Cout = default_output;
    last_instruction_was_an_unconditional_branch = false;
}

/**
 * Stop sending emitted code to a buffer.
 */
void CodeEmitter::endOutputToBuffer() {
    Cout = default_output;
}

/**
 *
 */
void CodeEmitter::emitUnconditionalBranch(const char *label, std::string const &debug_md, std::string const &is_parallel_md) {
    if (! last_instruction_was_an_unconditional_branch) {
        (*Cout) << indent() << "br label %" << label;
        last_instruction_was_an_unconditional_branch = true;
    }
    else  (*Cout) << commentIndent() << "br label %" << label; // continue to emit unneeded branch as a comment for now.
    (*Cout) << debug_md << is_parallel_md << endl;
}


void CodeEmitter::emitUnconditionalBranch(const char *label, std::string const &debug_md) {
  std::string empty;
  emitUnconditionalBranch(label, debug_md, empty);
}

/**
 *
 */
void CodeEmitter::emitUnconditionalBranch(string label, std::string const &debug_md, std::string const &is_parallel_md) {
    emitUnconditionalBranch(label.c_str(), debug_md, is_parallel_md);
}

void CodeEmitter::emitUnconditionalBranch(string label, std::string const &debug_md) {
  std::string empty;
  emitUnconditionalBranch(label, debug_md, empty);
}

/**
 *
 */
void CodeEmitter::emitLabel(FunctionAstAttribute *attribute, string label) {
    (*Cout) << label << ":";
    vector<string> &predecessors = attribute -> getLabelPredecessors(label);
    if (predecessors.size() > 0) {
        (*Cout) << "\t\t; preds = ";
        for (int i = 0; i < predecessors.size(); i++) {
            (*Cout) << "%" << predecessors[i];
            if (i < predecessors.size() - 1)
                (*Cout) << ", ";
        }
    }
    (*Cout) << endl;
    if (Cout == default_output) last_instruction_was_an_unconditional_branch = false;
}
