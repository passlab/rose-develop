#ifndef ROSE_Partitioner2_ModulesElf_H
#define ROSE_Partitioner2_ModulesElf_H

#include <Partitioner2/BasicTypes.h>
#include <Partitioner2/Function.h>
#include <Partitioner2/Modules.h>

namespace Rose {
namespace BinaryAnalysis {
namespace Partitioner2 {
namespace ModulesElf {

/** Reads ELF .eh_frames to find function entry addresses.
 *
 *  Performs an AST traversal rooted at the specified @p ast to find ELF .eh_frames sections and returns a sorted list of
 *  functions at unique starting addresses.  The functions are not attached to the CFG/AUM.
 *
 * @{ */
std::vector<Function::Ptr> findErrorHandlingFunctions(SgAsmElfFileHeader*);
std::vector<Function::Ptr> findErrorHandlingFunctions(SgAsmInterpretation*);
size_t findErrorHandlingFunctions(SgAsmElfFileHeader*, std::vector<Function::Ptr>&);
/** @} */

/** Reads ELF PLT sections and returns a list of functions.
 *
 * @{ */
std::vector<Function::Ptr> findPltFunctions(const Partitioner&, SgAsmElfFileHeader*);
std::vector<Function::Ptr> findPltFunctions(const Partitioner&, SgAsmInterpretation*);
size_t findPltFunctions(const Partitioner&, SgAsmElfFileHeader*, std::vector<Function::Ptr>&);
/** @} */

/** True if the function is an import.
 *
 *  True if the specified function is an import, whether it's actually been linked in or not. This is a weaker version of @ref
 *  isLinkedImport. */
bool isImport(const Partitioner&, const Function::Ptr&);

/** True if function is a linked import.
 *
 *  Returns true if the specified function is an import which has been linked to an actual function. This is a stronger version
 *  of @ref isImport. */
bool isLinkedImport(const Partitioner&, const Function::Ptr&);

/** True if function is a non-linked import.
 *
 *  Returns true if the specified function is an import function but has not been linked in yet. */
bool isUnlinkedImport(const Partitioner&, const Function::Ptr&);

/** Matches an ELF PLT entry.  The address through which the PLT entry branches is remembered. This address is typically an
 *  RVA which is added to the initial base address. */
struct PltEntryMatcher: public InstructionMatcher {
    rose_addr_t baseVa_;                                // base address for computing memAddress_
    rose_addr_t gotEntryVa_;                            // address through which an indirect branch branches
    size_t gotEntryNBytes_;                             // size of the global offset table entry in bytes
    rose_addr_t gotEntry_;                              // address read from the GOT if the address is mapped (or zero)
    size_t nBytesMatched_;                              // number of bytes matched for PLT entry

public:
    PltEntryMatcher(rose_addr_t base)
        : baseVa_(base), gotEntryVa_(0), gotEntryNBytes_(0), gotEntry_(0), nBytesMatched_(0) {}
    static Ptr instance(rose_addr_t base) { return Ptr(new PltEntryMatcher(base)); }
    virtual bool match(const Partitioner&, rose_addr_t anchor);

    /** Size of the PLT entry in bytes. */
    size_t nBytesMatched() const { return nBytesMatched_; }

    /** Address of the corresponding GOT entry. */
    rose_addr_t gotEntryVa() const { return gotEntryVa_; }

    /** Size of the GOT entry in bytes. */
    size_t gotEntryNBytes() const { return gotEntryNBytes_; }
    
    /** Value stored in the GOT entry. */
    rose_addr_t gotEntry() const { return gotEntry_; }

    // [Robb Matzke 2018-04-06]: deprecated: use gotEntryVa
    rose_addr_t memAddress() const { return gotEntryVa_; }
};

/** Build may-return white and black lists. */
void buildMayReturnLists(Partitioner&);

} // namespace
} // namespace
} // namespace
} // namespace

#endif
