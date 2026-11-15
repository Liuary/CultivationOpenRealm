# CORUI architecture and code review

## Scope, method, and overall verdict

Read-only static review of the requested CORUI runtime classes, JsEnvManager, PuertsMixinGenerator, plugin configuration, and TypeScript UI surface. Supporting reads included Puerts’ local `FJsObject` implementation, `CorStart.ts`, and the mixin import list. Only filesystem read/search commands were used; no files were modified and no builds or runtime tests were executed.

**Verdict: significant lifecycle and correctness work is needed before treating CORUI as a dependable UI framework.** The two known designer UAF fixes are present, but runtime ownership, initialization ordering, asynchronous completion, and callback re-entrancy remain substantial risks. No P0 finding was established.

Paths below are repository-relative. Findings distinguish directly observed defects from engine-dependent risks requiring runtime verification.

## Findings

### 1. P1 — Root UI ownership is not GC-visible

**Location:** `Plugins/CORUI/Source/CORUI/Public/CORUIManager.h:42`; `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:126`

**Issue and evidence:** The dynamically created root `UCanvasPanel` is retained only through `TWeakObjectPtr`; layer references are also weak. `InitUI` gives the viewport its Slate widget at line 168, but the manager declares no strong reflected root reference or reference-collector implementation.

This leaves no explicit UObject ownership chain rooted in the manager. A viewport-held Slate canvas is not an explicit GC reference to its originating `UCanvasPanel`. **The resulting collection behavior needs engine/runtime verification**, but the missing ownership is visible in the plugin.

**Recommended fix:** Hold the root through a transient `UPROPERTY` `TObjectPtr<UCanvasPanel>`. Audit and explicitly provide GC reference collection for the embedded `FUserWidgetPool`.

**Regression test:** Open windows, force repeated garbage collection, then push/pop and clear UI. Verify root, containers, active widgets, and inactive pooled widgets remain valid until deliberate teardown.

### 2. P1 — Async push reports pending loads as failures

**Location:** `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:334`

**Issue and evidence:** Immediately after `RequestAsyncLoad`, the code treats `!HasLoadCompleted()` as an error and calls the failure callback. A normal pending request can subsequently complete and call `NativePushUI`, producing a second, successful callback.

The completion lambda silently returns when `ResolveClass()` fails at line 327. `SyncPushUI` similarly returns without notifying failure at line 347.

**Recommended fix:** Establish an exactly-once completion contract. Handle request rejection separately from pending execution; report class resolution/type failures through the completion path.

**Regression test:** Exercise cold load, cached load, missing asset, wrong asset type, malformed path, and empty path. Assert exactly one terminal result per request.

### 3. P1 — Lifecycle callbacks can invalidate active stack traversal and push transactions

**Location:** `Plugins/CORUI/Source/CORUI/Private/UI/Container/CORCanvasContainer.cpp:296`; `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:303`

**Issue and evidence:** `UpdateWindowState` indexes the live `WindowsStack` while `NativeUpdateState` invokes overridable `OnCover`, `OnHide`, and `OnShow` callbacks. Those callbacks can synchronously push/pop windows, changing the array before the next indexed access.

Likewise, `NativePushUI` calls `NativeOpen`, updates state, and reports success without checking whether callbacks have already closed or replaced that window. `DetachWindow` calls `Hide` and `NativeClose` before finishing parent/Slate detachment at lines 198–203.

Critical sections do not establish a safe transaction boundary for same-thread callback re-entry.

**Recommended fix:** Queue mutations during lifecycle dispatch or use explicit transaction/generation guards. Traverse stable snapshots and revalidate membership after callbacks. Complete structural detachment before exposing close notifications where feasible.

**Regression test:** Have `OnOpen`, `OnShow`, `OnHide`, `OnCover`, and `OnClose` independently close themselves, remove lower windows, and push replacements. Assert no invalid indexing, duplicate release, or success callback for an already-closed activation.

### 4. P1 — UI and JS startup depend on competing next-tick callbacks

**Location:** `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:103`; `Plugins/CORUI/Source/JsEnvManager/Private/JsEnvManagerSubsystem.cpp:74`

**Issue and evidence:** Both subsystems independently schedule startup for the next tick. UI broadcasts `OnUIManagerInit` at line 178. JS subscribes only after environment startup reaches `CorStart.Start()` and `GameManager.BindCppEvent()`.

There is no ready-state replay or explicit startup handshake. **If UI initialization runs first, JS misses the only initialization notification.** Actual timer dispatch order was not verified.

**Recommended fix:** Use explicit orchestration: start JS, bind listeners, then initialize UI; alternatively expose an idempotent readiness query/subscription that immediately handles an already-ready manager.

**Regression test:** Deliberately execute both startup orders. Assert every TS manager initializes exactly once.

### 5. P1 — Teardown and world replacement lack a coordinated lifecycle boundary

**Location:** `Plugins/CORUI/Source/JsEnvManager/Private/JsEnvManagerSubsystem.cpp:25`; `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:107`

**Issue and evidence:** JsEnvManager resets the environment in its own `Deinitialize`; CORUI separately broadcasts JS-facing deinit and destructs widgets. No explicit shutdown sequence guarantees those callbacks execute before JS reset.

Both subsystems also react to every matching GameMode initialization. `InitUI` does not clear or reject an existing root, and `StartJsEnv` replaces an existing environment without first closing its UI. `UIEndPlayHandle` is declared but no corresponding world-end registration appears in the implementation.

**Recommended fix:** Define idempotent session states and coordinate shutdown: reject new work, invalidate requests, close widgets while JS is alive, release UI resources, then destroy JS. Explicitly handle world transition and repeated initialization.

**Regression test:** Repeat PIE start/stop and travel between maps in one GameInstance. Verify one root/environment, balanced lifecycle events, no surviving old viewport content, and no callbacks into a retired session.

### 6. P1 — Pending UI loads are not tied to the current UI session

**Location:** `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:318`

**Issue and evidence:** The weak delegate protects UObject destruction, but the manager can remain alive across UI clearing/reinitialization. Async push handles are local and are neither tracked nor cancelled by `ClearUI`.

A request from an old session can therefore complete against a newly created root and open an obsolete window. The `IsValid(this)` branch at lines 321–323 is empty and supplies no additional protection.

**Recommended fix:** Track pending handles and a session/request generation. Cancel or invalidate requests during teardown and validate generation before mutation or JS notification.

**Regression test:** Delay loading, tear down/recreate UI, then complete the old request. Assert it cannot create a window in the new session.

### 7. P2 — Re-pushing a retained fullscreen window duplicates its stack entry

**Location:** `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:268`; `Plugins/CORUI/Source/CORUI/Private/UI/Container/CORCanvasContainer.cpp:38`

**Issue and evidence:** Duplicate push calls pop with `bPopSelf=false`. For fullscreen windows, the container deliberately retains the target. The manager then reuses it and calls `PushUI`, which unconditionally appends it to `WindowsStack`.

Additionally, fullscreen pop uses `WindowIndex + 2` at line 98, preserving the immediately higher window even though the public API describes removal of windows above the target.

**Recommended fix:** Define duplicate-push semantics explicitly. Move an existing entry instead of appending it; enforce one entry per activation. Resolve the fullscreen pop offset against the intended contract.

**Regression test:** Push fullscreen A, then B/C, then re-push A repeatedly. Assert unique stack membership and exact expected survivors for both `bPopSelf` values.

### 8. P2 — Failed insertion leaves an acquired widget active in the pool

**Location:** `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:281`; `Plugins/CORUI/Source/CORUI/Private/CORUIManager.cpp:296`

**Issue and evidence:** After obtaining a widget from the pool, a failed container insertion invokes failure and returns without releasing the widget. Container insertion rejects `NAME_None` and invalid slots.

**Recommended fix:** Validate identifiers before acquisition and release newly acquired widgets on every insertion failure. Do not expose a failed activation as a usable window.

**Regression test:** Force empty effective name and slot insertion failure. Assert active pool count and lifecycle state return to their pre-request values.

### 9. P2 — Streamable handle release is unavailable through the registered JS surface

**Location:** `Plugins/CORUI/Source/CORUI/Private/Core/AutoRegister/AutoRegister.cpp:55`; `Plugins/CORUI/Source/CORUI/Public/Core/Streamable/CORStreamableManager.h:46`

**Issue and evidence:** Static registration exposes `RequestAsyncLoad` and `RequestAsyncLoadList`, but omits `ReleaseHandle`. The latter is not a `UFUNCTION`, either. Handles remain stored until explicit native release or subsystem deinitialization.

The loader also returns IDs for invalid handles, and `ReleaseHandle` removes an entry only when the stored handle is valid (`CORStreamableManager.cpp:73`).

**Recommended fix:** Expose release/cancel operations, reject failed requests, and remove map entries regardless of handle validity. Return structured completion status rather than an argument-free notification.

**Regression test:** From TS, request/release repeatedly, including invalid paths and empty lists. Assert bounded handle-map size and distinguish successful, failed, and cancelled requests.

### 10. P2 — Delayed TS callbacks outlive window activations

**Location:** `TypeScript/Game/UI/GameLoading/UI_GameLoading.ts:132`; `TypeScript/Game/UI/Test/UI_TestData.ts:92`

**Issue and evidence:** GameLoading schedules an untracked timeout that opens the hub and calls `this.CloseUI`; neither `OnClose` nor `Destruct` cancels it. TestData similarly schedules an untracked timeout that writes activation state after 500 ms.

With pooled reuse, the UObject can still exist while representing a later activation. Object validity alone would not prevent stale behavior.

**Recommended fix:** Track and cancel every activation-owned timer in close/destruct, and validate an activation generation inside delayed callbacks.

**Regression test:** Schedule the callback, close and immediately reopen the same pooled class, then advance time. Assert the old callback cannot close or modify the new activation.

### 11. P2 — Designer module shutdown does not reach live extensions

**Location:** `Plugins/CORUI/Source/PuertsMixinGenerator/Private/PmgDesignerExtensionFactory.cpp:17`; `Plugins/CORUI/Source/PuertsMixinGenerator/Private/PuertsMixinGenerator.cpp:164`

**Issue and evidence:** `CleanupActiveExtensions` traverses `ActiveExtensions`, but the factory only creates an extension; source search found no registration into that array. Consequently, module cleanup does not reach the created extensions.

Even if registered, `FPmgDesignerExtension::Cleanup` at line 282 only removes delegates and clears VBox references; it does not invalidate the deferred initialization token or remove the raw ticker.

**Recommended fix:** Register live extensions or centralize ownership in the factory. Make cleanup terminal and idempotent: invalidate deferred work, unregister delegates, remove ticker, and release UI state.

**Regression test:** Keep designers open while exercising supported module shutdown/reload. Verify no ticker, delegate, deferred callback, or extension remains able to execute module code afterward. **Module-unload crash behavior was not reproduced.**

### 12. P2 — Mixin generation dereferences a class after explicitly allowing it to be missing

**Location:** `Plugins/CORUI/Source/PuertsMixinGenerator/Private/PuertsMixinGenerator.cpp:285`

**Issue and evidence:** Generation handles a null `Blueprint->GeneratedClass` with a fallback asset path at lines 243–250, then unconditionally dereferences it at lines 285, 292, and 301.

File saves at lines 311 and 347 also ignore their return values, so generation failure is silent.

**Recommended fix:** Require a valid generated class before generating class-specific content, or consistently guard all accesses. Check directory creation and save results and report actionable errors.

**Regression test:** Generate from an uncompiled/invalid Blueprint and a non-writable destination. Assert a clear error and no crash or misleading success.

### 13. P2 — Script initialization accepts nullable/untyped data after opening the window

**Location:** `TypeScript/Manager/UIManager.ts:60`; `TypeScript/Game/UI/GameStart/UI_GameStart.ts:44`

**Issue and evidence:** UIManager checks only truthiness before calling `OnScriptInit`, rather than checking that it is a function. Its payload defaults to null, but GameStart immediately dereferences `Single`.

C++ opens and updates window visibility before the callback invokes `OnScriptInit`. Thus lifecycle code runs before payload validation, and a script initialization exception leaves the window already inserted/open.

**Recommended fix:** Define a typed script contract, validate callable hooks and payloads, and initialize activation data before open/show. Return initialization failure explicitly and roll back failed activations.

**Regression test:** Supply null/malformed payloads, absent/non-function hooks, and a throwing initializer. Verify predictable failure with no partially initialized active window.

### 14. P3 — Debug FPS measures timer cadence

**Location:** `TypeScript/Game/UI/Test/UI_TestDebug.ts:55`; `TypeScript/Game/UI/Test/UI_TestDebug.ts:77`

**Issue and evidence:** Refresh runs every 250 ms and labels `1000 / elapsed` as FPS. At stable scheduling it reports approximately 4, independently of rendering frame rate.

**Recommended fix:** Read actual frame timing or label the value as refresh cadence.

**Regression test:** Compare the display at controlled 30/60 FPS; it should reflect rendering rate or explicitly identify timer timing.

## Architecture

The principal separation is understandable:

| Component | Observed responsibility |
|---|---|
| `UCORUIManager` | Root/layer creation, configuration, class loading, widget pooling, public push/pop, JS completion |
| `UCORCanvasContainer` | Window registry/stack, ZOrder, detachment, pool return, visibility-state traversal |
| `UCORWindow` / `UCORUserWidget` | Lifecycle events, visibility and cover state, child lifecycle propagation |
| `UCORStreamableManager` | Separate asset-loading API with integer handle storage |
| JsEnvManager | Environment creation and GameMode-triggered startup |
| TS UIManager | Subsystem lookup and dynamic `OnScriptInit` dispatch |

The boundaries are weakened by duplicated loading paths and direct JS types in the runtime manager. Window self-close resolves the global subsystem (`CORWindow.cpp:140`), while containers retain a raw pointer into manager-owned pool storage (`CORCanvasContainer.h:95`).

Recommended direction: a game-thread UI coordinator with explicit session/activation state, an injectable loading interface, and a separate JS adapter. Stack/state policy can then be tested without constructing a Puerts environment.

The C++ button, text block, and progress bar are currently empty UMG subclasses. `CORImage` adds path loading but delegates async behavior to `UImage`; its engine-level cancellation behavior was not verified.

## Lifetime and ownership

The two reported designer fixes are present:

- `OnAssetClosedInEditor` is removed during uninitialization and destruction (`PmgDesignerExtension.cpp:27`, `:96`, `:149`).
- Deferred initialization uses a weak lifetime token, weak designer Slate reference, and weak Blueprint reference (`:72–89`). The lifetime check precedes dereferencing `this` through short-circuit evaluation.

Remaining concerns are findings 1, 3, 5, 10, and 11. The designer destructor removes its ticker; uninitialization sets `bIsDestroyed`, causing its next tick to stop. Module cleanup is less complete.

Runtime next-tick delegates use `CreateUObject`, which addresses destroyed-UObject invocation, but their handles are discarded and there is no explicit session-state guard. This differs from protecting against callbacks after logical deinitialization.

## Async and threading

The reviewed code contains no explicit worker-thread UI mutation. It relies on engine callback scheduling; no game-thread assertions enforce that contract. Critical sections should not be interpreted as making UMG or JS calls safe from arbitrary threads.

The main verified async defects are pending-as-failure handling, inconsistent terminal notification, missing JS release exposure, and missing session/activation cancellation.

Core window-state updates are mutation-driven, not per-frame: `NativeUpdateUI` traverses all layers after pushes, and container pops update their own layer. The manager resets state to `None` for each layer (`CORUIManager.cpp:398`), so cross-layer hiding/covering is not propagated. Whether that is intended needs a product-level contract.

Within a layer, membership checks and removals are linear; bulk removal can become quadratic. Designer diagnostics recursively allocate child arrays and sort canvas children on each ticker update. These are observable costs, but no profiling was performed.

## JS bridge

Local Puerts `FJsObject::Action` checks environment lifetime, rejects non-functions, and catches/logs JS exceptions (`Plugins/Puerts/Source/JsEnv/Public/JsObject.h:159`, `:174`, `:184`). Consequently, an expired captured callback should not automatically be classified as an isolate UAF.

Those safeguards do not provide request completion, rollback, or activation validity. CORUI currently supplies none of those contracts consistently.

Mixin imports synchronously load Blueprint classes at module evaluation; `CorStart.ts:4` imports the complete list before binding manager events. This both adds startup work and contributes to the readiness ordering problem. Type assertions and `UISingle.GetSingle<T>` provide no runtime validation.

## Top five risks and follow-up requirements

| Priority | Risk | REQ candidate |
|---|---|---|
| 1 | Root/pool ownership under GC | **REQ-UI-OWN-001:** Every live UI UObject and pooled instance must have a documented, testable GC ownership chain. |
| 2 | Callback re-entry corrupts activation/stack state | **REQ-UI-TXN-002:** Lifecycle callbacks may request mutations without invalid traversal, duplicate membership, or double release. |
| 3 | Startup/teardown ordering loses TS lifecycle events | **REQ-UI-LIFE-003:** Initialization and shutdown must follow one idempotent sequence across JS, UI, and world changes. |
| 4 | Async requests produce duplicate or stale results | **REQ-UI-ASYNC-004:** Each request must complete exactly once and be bound to a cancellable UI session. |
| 5 | Editor extensions survive module cleanup | **REQ-EDITOR-LIFE-005:** Every extension’s delegates, ticker, and deferred work must be disabled before supported module teardown. |

## Gaps and uncertainties

- No runtime reproduction, automated tests, build, packaged execution, or profiling was performed.
- The binary `Content/Config/DA_UILayerTypeZOrderConfigData.uasset` was located but its serialized values were not decoded. Project configuration points to it; actual ZOrder values remain unverified.
- Engine implementations of widget-pool GC, Slate/UObject retention, timer ordering, streamable cancellation, and `UImage` async replacement were not read. Engine-dependent consequences are identified above.
- Actual subsystem deinitialization order was not verified; the plugin lacks an explicit coordination mechanism regardless.
- Blueprint widget bindings, inheritance, animations, and asset validity were not inspected. TS declarations alone do not establish their runtime correctness.
- Existing TS test screens provide lifecycle counters and manual exercises, but their presence is not evidence that the proposed regressions pass.
- Cross-layer cover policy, equal layer-ZOrder behavior, and the intended fullscreen `bPopSelf=false` semantics require explicit acceptance criteria.
