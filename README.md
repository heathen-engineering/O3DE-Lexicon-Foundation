# Heathen Lexicon Localisation Gem
![License](https://img.shields.io/badge/License-Apache_2.0-blue?style=flat-square)
![Maintained](https://img.shields.io/badge/Maintained%3F-yes-green?style=flat-square)
![O3DE](https://img.shields.io/badge/O3DE-25.10%20%2B-%2300AEEF?style=flat-square&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAxTDEgNy40djkuMkwxMiAyM2wxMS02LjRWNy40TDEyIDF6bTkuMSAxNC45TDExLjUgMjEuM2wtOC42LTYuNFY4LjFsOC42LTYuNCA5LjEgNi40djYuOHpNMTEuNSA0LjZMMi45IDkuNnY0LjhsOC42IDUuMSA4LjYtNS4xVjkuNmwtOC42LTUuMHoiLz48L3N2Zz4=)

An [Open 3D Engine (O3DE)](https://o3de.org) gem providing a deterministic, asset-driven localisation system. Decouples logical dot-path addresses from runtime values using xxHash — supporting strings, sound assets, and any other asset type, not just text.

- **License:** Apache 2.0
- **Origin:** Heathen Group
- **Platforms:** Windows, Linux, Android, iOS

---

## Become a GitHub Sponsor
[![Discord](https://img.shields.io/badge/Discord--1877F2?style=social&logo=discord)](https://discord.gg/6X3xrRc)
[![GitHub followers](https://img.shields.io/github/followers/heathen-engineering?style=social)](https://github.com/heathen-engineering?tab=followers)
Support Heathen by becoming a [GitHub Sponsor](https://github.com/sponsors/heathen-engineering). Sponsorship directly funds the development and maintenance of free tools like this, as well as our game development [Knowledge Base](https://heathen.group/) and community on [Discord](https://discord.gg/6X3xrRc).

Sponsors also get access to our private SourceRepo, which includes developer tools for O3DE, Unreal, Unity, and Godot.
Learn more or explore other ways to support @ [heathen.group/kb](https://heathen.group/kb/do-more/)

---

## Core Concept

Each language/culture is stored as a **HeLex** file pair:

| Extension | Role | Contents |
|-----------|------|----------|
| `.helex` | Source | Human-readable JSON; edited by developers and localisation teams |
| `.lexicon` | Product | Binary `LexiconAssemblyAsset`; produced by the Asset Processor |

At runtime the system component holds the active and default `LexiconAssemblyAsset`. Lookups are O(log n) binary search on a sorted hash index. The hash algorithm is **XXH3\_64bits** (seed 0) — the same algorithm used across the Heathen gem suite.

A single `.helex` file can service **multiple culture codes** (e.g. `"pt-BR"` and `"es-419"`). There is no enforced folder structure; place `.helex` files anywhere in the project source tree.

---

## .helex File Format

A `.helex` file is a UTF-8 JSON document with two top-level fields.

### Schema

```json
{
  "cultures": [ "<culture-code>", ... ],
  "entries":  { "<dot.path.key>": <value>, ... }
}
```

### `cultures`

An array of [BCP 47](https://www.rfc-editor.org/rfc/rfc5646) culture codes this file services.

```json
"cultures": ["en-GB", "en-IE"]
```

A lexicon is selected at runtime by calling `LoadCulture("en-GB")`. The system scans all registered `.lexicon` assets and loads the first one whose `cultures` list contains the requested code.

### `entries`

A flat object mapping dot-path keys to values. Keys may use any depth of dot-notation.

#### String value (LexiconText)

A plain JSON string becomes a localised text entry.

```json
"Menu.Play":         "Play",
"Menu.Settings":     "Settings",
"HUD.Health.Label":  "Health"
```

#### Asset reference value (LexiconAsset / LexiconSound)

A JSON object with a single `"uuid"` field becomes an asset ID entry. The value is a standard O3DE asset UUID in `{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}` format. The Asset Processor will declare this UUID as a **product dependency**, preventing it from being stripped in packaged builds.

```json
"Audio.Music.Theme": { "uuid": "{1A2B3C4D-5E6F-7890-ABCD-EF1234567890}" },
"UI.Logo":           { "uuid": "{AABBCCDD-EEFF-0011-2233-445566778899}" }
```

### Full example

```json
{
  "cultures": ["en-GB", "en-IE"],
  "entries": {
    "Menu.Play":          "Play",
    "Menu.Settings":      "Settings",
    "Menu.Quit":          "Quit",
    "HUD.Health.Label":   "Health",
    "HUD.Stamina.Label":  "Stamina",
    "Audio.Music.Theme":  { "uuid": "{1A2B3C4D-5E6F-7890-ABCD-EF1234567890}" },
    "UI.Logo":            { "uuid": "{AABBCCDD-EEFF-0011-2233-445566778899}" }
  }
}
```

---

## Hashing Rule

Keys are hashed at **build time** (by the Asset Processor) and at **runtime** (by the convenience string overloads on the bus). The algorithm must be identical in both places:

```
hash = XXH3_64bits_withSeed(key.data(), key.size(), /*seed=*/0)
```

Via the xxHash gem's C++ API:

```cpp
#include <xxHash/xxHashFunctions.h>
AZ::u64 hash = xxHash::xxHashFunctions::Hash64("Menu.Play", 0);
```

Never use `XXH64` or `XXH32` for key hashing — they produce different values and lookups will silently miss.

---

## Runtime API

### C++ — via the EBus

```cpp
#include <FoundationLocalisation/FoundationLocalisationBus.h>

// Load a culture (async — resolution returns empty until load completes)
FoundationLocalisation::FoundationLocalisationRequestBus::Broadcast(
    &FoundationLocalisation::FoundationLocalisationRequests::LoadCulture, "en-GB");

// Resolve a string by dot-path (hashes internally)
AZStd::string label;
FoundationLocalisation::FoundationLocalisationRequestBus::BroadcastResult(
    label, &FoundationLocalisation::FoundationLocalisationRequests::ResolveString, "Menu.Play");

// Resolve by pre-cached hash (preferred in hot paths)
AZ::u64 hash = xxHash::xxHashFunctions::Hash64("Menu.Play", 0);
FoundationLocalisation::FoundationLocalisationRequestBus::BroadcastResult(
    label, &FoundationLocalisation::FoundationLocalisationRequests::ResolveString, hash);

// Resolve an asset UUID
AZ::Uuid assetId;
FoundationLocalisation::FoundationLocalisationRequestBus::BroadcastResult(
    assetId, &FoundationLocalisation::FoundationLocalisationRequests::ResolveAssetId, "Audio.Music.Theme");
```

### C++ — via the AZ::Interface (lowest overhead)

```cpp
if (auto* loc = FoundationLocalisation::FoundationLocalisationInterface::Get())
{
    AZStd::string label  = loc->ResolveString("Menu.Play");
    AZ::Uuid      soundId = loc->ResolveAssetId("Audio.Music.Theme");
}
```

---

## Data Types

Use these types as fields on your components to make individual values localisation-aware.

### `Heathen::LexiconText`

A localisation-aware string. Include `<FoundationLocalisation/LexiconText.h>`.

```cpp
#include <FoundationLocalisation/LexiconText.h>

class MyComponent : public AZ::Component
{
    Heathen::LexiconText m_buttonLabel; // serialises into the Inspector
};
```

In the Inspector the field shows a mode toggle and either a key picker (Localised) or a plain text field (Literal/Invariant).

| Mode | Behaviour |
|------|-----------|
| `Localised` | `m_keyOrValue` is a dot-path key; resolved at runtime via the bus |
| `Literal` | `m_keyOrValue` is a raw string; no lookup performed |
| `Invariant` | Same as Literal; excluded from the Gatherer batch-conversion tool |

**Resolve in code:**

```cpp
// Option A — let the bus hash for you
AZStd::string value;
FoundationLocalisation::FoundationLocalisationRequestBus::BroadcastResult(
    value, &FoundationLocalisation::FoundationLocalisationRequests::ResolveString,
    m_buttonLabel.IsLocalised() ? m_buttonLabel.GetHash() : 0);

// Option B — check mode manually
if (m_buttonLabel.IsLocalised())
    value = loc->ResolveString(m_buttonLabel.GetHash());
else
    value = m_buttonLabel.m_keyOrValue;
```

### `Heathen::LexiconSound`

A localisation-aware sound asset reference. Localised mode resolves to an `AZ::Uuid` (asset ID). Include `<FoundationLocalisation/LexiconSound.h>`.

```cpp
Heathen::LexiconSound m_backgroundMusic;

// Resolve
AZ::Uuid soundId = m_backgroundMusic.IsLocalised()
    ? loc->ResolveAssetId(m_backgroundMusic.GetHash())
    : m_backgroundMusic.m_literalAssetId;
```

### `Heathen::LexiconAsset`

A localisation-aware generic asset reference (texture, prefab, font, etc.). Same interface as `LexiconSound`. Include `<FoundationLocalisation/LexiconAsset.h>`.

---

## Culture vs. Fallback

- **Active culture** — loaded via `LoadCulture("en-GB")`. Checked first on every lookup.
- **Default culture** — set via `SetDefaultCulture("en-US")`. Checked when the active lexicon has no entry for a key.
- On a total miss (neither lexicon has the key) `ResolveString` returns an empty string; `ResolveAssetId` returns a null `AZ::Uuid`.

---

## Requirements

- O3DE engine **25.10.2** or compatible
- `xxHash` gem (included in this project; provides `xxHash::xxHashFunctions`)
