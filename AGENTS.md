# AGENTS

## Code Style: Git Commit Convention

The project follows [Conventional Commits](https://www.conventionalcommits.org/) for git history clarity and automation.

- Use prefixes indicating the type of change:
  `fix:`, `refactor:`, `example:`, `test:`, `docs:`, `feat:`, etc.
- Optional scope in parentheses:
  `fix(include):`, `refactor(server):`, `example(codex):`
- Commit message is short and imperative:
  Examples: `fix(include): remove redundant header`, `refactor(server): simplify transaction logic`

Format:
```text
type(scope): short message
```

## Naming Conventions

- Prefix `m_` is required for class fields (e.g., `m_event_hub`, `m_task_manager`).
- Prefixes `p_` and `str_` are optional and can be used when a function or method has more than five variables or arguments of different types.
- Boolean variables should start with `is`, `has`, `use`, `enable` or with `m_is_`, `m_has_`, etc. for class fields.
- Do not use prefixes `b_`, `n_`, `f_`.

### Domain DTO Naming (all domains)

- Reuse the tick-domain naming template for every DTO-oriented domain:
  - **DTO types:** `<Base><Suffix><Kind>`, where `Kind` is the domain noun (`Tick`, `Bar`, `Quote`, `Order`, etc.), `Base` is the entity (`Value`, `Market`, `Funding`), and `Suffix` is an optional short modifier (`Vol`, `L1`, `Agg`). Examples: `ValueTick`, `FundingRateBar`, `OrderBookL2Snapshot`.
  - **Containers / algorithms:** `<Kind><Something>` to emphasize operations over that domain (e.g., `TickSequence`, `BarCompressorV2`, `OrderSerializer`).
  - **Span aliases:** `<TypeName>Span` for zero-copy ranges, such as `ValueTickSpan`, `PriceBarSpan`, `OrderBookSnapshotSpan`.
- For the tick domain, additional rationale and examples remain in `include/DataFeedHub/data/ticks/README-RU.md`; other domains should mirror this structure in their own READMEs when new DTO families appear.

### Constants and compile-time flags

- **Preprocessor macros / feature toggles** use `UPPER_SNAKE_CASE` and usually include a project prefix, e.g., `DFH_USE_JSON`, `DFH_USE_NLOHMANN_JSON`, `DFH_STORAGE_ENABLE_MDBX`.
- **`static constexpr` / `const` data inside classes and structs** also use `UPPER_SNAKE_CASE`, e.g., `TRADE_SIDE_BITS`, `DEFAULT_CAPACITY`.
- When referencing those `static constexpr` / `const` members (or namespace-level constants), always qualify them with their scope (`TradeTick::TRADE_SIDE_BITS`, `ticks::TickCodecConfig::DEFAULT_CAPACITY`) to keep usage distinguishable from macros.
- **Regular data members** keep `snake_case` names such as `price`, `time_ms`, `id_and_side`.
- **Enum constants** use `CamelCase` (`Unknown`, `Buy`, `Sell`).

## Feature toggles

- `DFH_USE_JSON` enables JSON helpers across the code base. When this macro is not defined the library never includes any JSON headers and the serialization helpers are not compiled.
- `DFH_USE_NLOHMANN_JSON` enables adapters built on top of [nlohmann/json](https://github.com/nlohmann/json). Define this macro together with `DFH_USE_JSON` to compile the `to_json`/`from_json` overloads and ADL serializers.
- `DFH_USE_SIMDJSON` reserves hooks for simdjson-based readers/writers. Combine it with `DFH_USE_JSON` when integrating the fast DOM/on-demand parsers. Both JSON backends may coexist when desired.
- Every domain that exposes DTOs must wrap its JSON serialization/deserialization helpers with these macros so that downstream projects can opt-in to JSON support explicitly.

## Documentation / Doxygen Style Guide

Applies to all C++11/14/17 sources in this repository.

- Prefer triple-slash fences: `///`. Avoid `/** ... */` unless required.
- Use backslash-style tags.
- Lines should generally stay under 100–120 columns and remain concise, technical, and in English.
- Do not start descriptions with "The".

### Tag order

Use the following ordering template:

```text
\brief
\tparam (each template parameter)
\param  (in function signature order)
\return (exactly once for non-void)
\throws
\pre
\post
\invariant
\complexity
\thread_safety
\note / \warning
```

- Every function parameter must have a matching `\param` with the same name and order.
- Every template parameter must have a matching `\tparam`.
- Non-void functions must document the return value with a single `\return`.
- Use `\throws` for each exception type.
- Add `\pre` and `\post` when meaningful, `\invariant` for class invariants.
- Document algorithmic complexity via `\complexity`.
- State `\thread_safety` as one of: "Thread-safe", "Not thread-safe", or "Conditionally thread-safe: …".
- Employ `\note` and `\warning` as needed.
- Public-facing docs should avoid unnecessary internal layout details.

### Examples

```cpp
/// \brief Resize canvas to the target dimensions.
/// \param w Width in pixels.
/// \param h Height in pixels.
/// \pre w >= 0 && h >= 0.
/// \post New size equals (w, h).
void resize(int w, int h);

/// \brief Computes 64-bit hash of the input.
/// \tparam T Input type supporting contiguous byte access.
/// \param data Input value.
/// \return 64-bit hash.
/// \complexity O(n) over input size.
/// \thread_safety Not thread-safe.
template<class T>
uint64_t hash(const T& data);
```

**Compliance:** If legacy comments conflict with this guide, this section overrides them for new or updated code.

## File Names

- If a file contains only one class, use `CamelCase` (e.g., `TradeManager.hpp`).
- If a file contains multiple classes, utilities, or helper structures, use `snake_case` (e.g., `trade_utils.hpp`, `market_event_listener.hpp`).

## Entity Names

- Class, struct, and enum names use `CamelCase`.
- Method names use `snake_case`.

## Method Naming

- Methods should be named in `snake_case`.
- Getter methods may omit the `get_` prefix when they simply return a reference or value or when they provide access to an internal object and behave like a property (e.g., `size()`, `empty()`).
- Use the `get_` prefix when the method performs computations to produce the returned value or when omitting `get_` would be misleading.

## Avoiding problematic thread_local usage

### Thread-local issue and SerializeScratch

Originally, temporary buffers for serialization used `thread_local` STL containers
(e.g., `std::vector<uint8_t>`). On Windows/MinGW this led to **heap corruption**
and random crashes at thread shutdown, because:

- `thread_local` destructors run when the CRT/heap may already be partially finalized
- STL containers may free memory using a different heap arena than the one they were created in
- destructor order across `thread_local` objects is not guaranteed

To fix this, we replaced all `thread_local` STL buffers with a dedicated helper:

```cpp
struct SerializeScratch {
    alignas(8) unsigned char small[16];
    std::vector<uint8_t> bytes;
    // ...
};
```

* `small[16]` provides a stack-like inline buffer for INTEGERKEY and other small values
* `bytes` is used for larger or variable-sized data, owned by the calling scope
* Returned `MDBX_val` is valid only until the next serialization call on the same scratch

This approach removes dependency on `thread_local` destructors, is portable across compilers,
and avoids MinGW-specific runtime crashes.

Prefer this `SerializeScratch`-style approach over `thread_local` STL containers everywhere in the code base.

## Domain-driven design (DDD)

The library is structured using DDD. High-level namespaces map to the domain boundaries:

```
include/
└── DataFeedHub/
    ├── compression/
    │   ├── bars/
    │   ├── bars.hpp
    │   ├── ticks/
    │   ├── ticks.hpp
    │   ├── utils/
    │   └── utils.hpp
    ├── compression.hpp
    ├── core/
    │   ├── data_feed/
    │   └── funding/
    ├── data/
    │   ├── bars/
    │   ├── bars.hpp
    │   ├── common/
    │   ├── common.hpp
    │   ├── funding/
    │   ├── funding.hpp
    │   ├── ticks/
    │   ├── ticks.hpp
    │   ├── trading/
    │   └── trading.hpp
    ├── data.hpp
    ├── dfh.hpp
    ├── storage/
    │   ├── common/
    │   │   ├── enums.hpp
    │   │   ├── flags.hpp
    │   │   ├── interfaces/
    │   │   │   ├── IConfig.hpp
    │   │   │   ├── IConnection.hpp
    │   │   │   ├── IMarketDataStorage.hpp
    │   │   │   └── ITransaction.hpp
    │   │   ├── interfaces.hpp
    │   ├── common.hpp
    │   ├── factory.hpp
    │   ├── mdbx/
    │   ├── mdbx.hpp
    │   ├── sqlite3/
    │   └── sqlite3.hpp
    ├── storage.hpp
    ├── transform/
    ├── transform.hpp
    ├── utils/
    └── utils.hpp
```

When adding new features, align them with the relevant domain layer and keep boundaries clean.

## Umbrella headers

To speed up compilation of this header-only library, create umbrella headers for each domain area (e.g., `ticks.hpp` gathers required dependencies for all tick-related structures). Prefer including the umbrella header at call sites instead of individual low-level headers to reduce repetitive include graphs.

- When a domain is expected to be consumed **only** through its umbrella header, move every transitive dependency (STL headers, cross-domain enums, local helpers, etc.) into the umbrella header. Leaf headers inside that domain should avoid `#include` statements entirely so that the umbrella controls ordering and compilation cost.
- Composite headers (where a single file defines a class along with its helpers, inline implementation, or nested utilities) may still include neighboring files from the same domain, because that header already acts as a small umbrella for its implementation details.
- When in doubt, default to “umbrella owns dependencies”: assume a leaf header is not meant to be included directly unless explicitly documented otherwise.
