# Braindance → Product Ideas

Braindance, from *Cyberpunk 2077*: a recording technology that captures a
person's full sensory and emotional experience so someone else can play it
back and *feel* it — not watch it. The editor around it lets you scrub time,
freeze any instant, walk around inside the frozen moment, and isolate
individual sensory layers (visual, audio, thermal, EM) to find things the
recorder never consciously noticed.

That's a genuinely reusable interaction pattern, and it doesn't require any
of the game's technology to exist. This document decomposes it into
primitives, scores a set of product ideas against them across several
domains, and closes with a dedicated game concept, playable on screen or in
VR.

## 1. Thesis — why braindance ≠ "recording"

Ordinary recordings are: deliberately started, single-layer, consumed
linearly, and analytically inert. Braindance inverts all four:

1. **Capture by default** — you decide the moment mattered *afterward*, not
   before. The system was already recording.
2. **Decompose into layers** — the insight rarely comes from the raw feed. It
   comes from isolating one channel (sound alone, heat alone, one person's
   voice) and looking at *only* that.
3. **Navigate non-linearly, including freezing** — time stops completely, but
   space stays live. You can walk through a frozen explosion.
4. **Transfer experience, not summary** — the recipient doesn't read a report
   about what happened. They feel what the recorder felt.

A product that adopts all four gets the braindance property. A product that
adopts one is just a screen recorder with extra steps.

There's a fifth property worth calling out on its own, because it's the
least obvious and the most under-used commercially:

5. **The recording is subjective.** A braindance carries the recorder's inner
   state, and that state colors what was captured — a frightened witness's
   memory renders a room darker than it was; a guilty party's memory edits
   their own actions into a kinder shape. This is simultaneously the medium's
   unique value (it transmits *feeling*, not just fact) and its unique
   unreliability (two recordings of the same event can genuinely disagree).
   Almost nothing on the market treats distortion as a *feature* rather than
   a bug to be corrected away.

## 2. The nine primitives

| Primitive | Game mechanic | Product-side equivalent |
|---|---|---|
| Retroactive capture | BD recorder is always rolling | Always-on ring buffer; you decide after the fact what to keep |
| First-person fidelity | Full sensory/emotional POV | Capture including internal/biometric state, not just screen+audio |
| Non-linear scrub | Timeline slider, jump anywhere | Random-access timeline UI over the captured stream |
| Layer separation | Visual / audio / thermal / EM tracks | Independent, isolatable channels of the same event |
| Frozen-moment free-roam | Time stops, camera doesn't | Freeze playback, let the viewer move through the reconstructed scene |
| Annotation → evidence | Tag a clue on the timeline | Marker becomes a citable, exportable artifact |
| Transfer of experience | Someone else *feels* your BD | Empathy/training transfer, not just information transfer |
| Subjective distortion | Fear/guilt warp what's recorded | Cross-referencing multiple POVs of one event exposes the truth |
| Dosage & harm | "Hot BDs" cause real damage | Every capture medium here has a consent economy and intensity limits |

## 3. Scoring rubric

Every idea below is scored against four questions:

| Question | Why it matters |
|---|---|
| Does the capture substrate already exist? | If you have to invent new sensors first, you're not building a product, you're building a platform. |
| Is there an identifiable buyer? | "Everyone" is not a buyer. Name the role that pays. |
| Does the BD framing beat the incumbent, or just rename it? | Screen recording, session replay, and video tutorials already exist. The pitch has to be *better*, not *cooler-sounding*. |
| How thin can v1 be? | If v1 needs all nine primitives simultaneously, it's a research project, not a product. |

## 4. Flagship: the game — working title *Recall* (screen + VR)

The most literal and, on inspection, the most viable product of a game
mechanic is *another game*. This is the deepest idea in the set.

**Core loop.** Each case hands you recorded memories — full 3D scenes,
30 seconds to 3 minutes long. You scrub time freely on a timeline; **freeze
any instant and free-roam the frozen scene** (walk through it, crouch under
a thrown glass mid-air, read a letter someone was holding); **switch
layers** — visual / audio (isolate one voice, footsteps, a phone buzzing in
someone's pocket) / thermal (recently touched objects, a person hiding
behind a curtain) / EM (devices, a hidden wire) / **emotional** (the
recorder's affect track — spikes mark what mattered *to them*, including
moments the visual layer makes look mundane). Tagging the right thing, in
the right layer, at the right moment produces evidence; evidence combines
into deductions that unlock the next memory.

**The novel mechanic: subjective memory.** Recordings come from people, and
people distort. A frightened witness renders the room darker and the
attacker taller than they were. A guilty party edits their own actions into
a kinder shape. An infatuated recorder literally cannot register the
victim's flaws. The player gets *multiple* recordings of the *same event*
from different POVs, each warped differently — and **the contradictions are
the clues**. Where two memories disagree, someone's emotional layer explains
why, and triangulating the distortions reconstructs what actually happened.
No shipped game makes distortion mechanical like this; it's the one clearly
untapped idea in the whole space. (Rashomon supplies the narrative
structure; Obra Dinn supplies the deduction engine; neither combines them
with a literal, playable distortion layer.)

**Screen vs. VR design.**
- *Screen:* timeline scrubber + free orbit camera in frozen moments; layers
  on hotkeys; a deduction board UI to combine evidence. Fully playable on
  its own — this is what ships first.
- *VR:* the mechanic VR was made for is **standing inside a frozen moment**
  — that's the demo that sells the headset version. Comfort solution that
  doubles as a design idea: memories play back at **tabletop diorama
  scale** while time is running (a god's-eye view, so nothing induces motion
  sickness), and **expand to full human scale only once frozen** — the
  world never moves while you're inside it, so free walking is always safe.
  Scrubbing is a physical gesture (grab and drag a floating timeline
  ribbon, or turn a jog dial). Layers switch from a wrist menu.

**Comparables and the gap.** *Return of the Obra Dinn* (deduction from
frozen death-moments), *Immortality* (scrubbing footage for a hidden truth),
*Tacoma* (frozen-time spatial replay of recorded people), *Her Story*
(non-linear archive detective work). Each proves one pillar of this sells on
its own; none combines scrub + free-roam + layer forensics + subjective
distortion. *Tacoma*'s modest commercial performance is the cautionary
lesson to take seriously: it nailed spatial replay but had no layers, no
real deduction stakes, no fail state — the takeaway is that the editor
fantasy needs a genuine detective game wrapped around it, not just a walking
sim with better staging.

**Art direction: point cloud / photogrammetry, not rendered environments.**
Every scene is presented as a colored point cloud — LiDAR/photogrammetry
scan aesthetic, luminous points floating in pure black, no solid geometry,
no textures. This isn't a stylistic add-on; it does real mechanical and
thematic work:

- **It sells the fiction for free.** A point cloud against black reads
  instantly as "reconstructed data," not "a place." The player never
  mistakes the memory for reality, which is exactly the distinction the
  whole game hinges on — you are never *in* the event, you are inside its
  *recording*.
- **Chromatic aberration = distortion, made visible.** Rather than a
  post-process filter applied uniformly, aberration intensity is driven by
  *how warped that memory is at that instant* — a calm, accurate stretch of
  a recording renders clean and tightly registered; a moment the recorder
  was frightened or lying about smears color channels apart, edges
  fringing red/cyan or magenta/green. The player learns to *read*
  aberration as an evidence signal, not just ambience — heavy fringing
  becomes a clue that something at that instant doesn't match the recorded
  person's account.
- **Palette encodes layer, not just scene.** Point color temperature
  reassigns per active layer rather than staying naturalistic: thermal
  layer runs a warm magenta/orange/gold palette (heat-map-as-humans-and-
  objects, per the reference mood — hot bodies and recently touched things
  glow orange against cool purple/blue structure); EM layer runs cool
  cyan/violet with points appearing only around active devices and wiring;
  the emotional layer desaturates everything *except* the recorder's
  affect spikes, which bloom in a single hot accent color at the exact
  moment they occur. Visual/audio layers stay closer to a naturalistic
  cool blue-green/tan point-cloud palette (per the landscape reference) so
  the "warped" layers read as a deliberate departure, not just a different
  filter.
- **Density communicates confidence.** A moment reconstructed from a
  clear, confident memory renders as a dense, stable cloud. A moment the
  recorder barely registered, or that's contested by another POV, renders
  sparse and slightly noisy/flickering — before the player even isolates a
  layer, cloud density is already hinting at which instants are worth
  freezing.
- **Cheap to build relative to the payoff.** Point-cloud rendering is
  computationally forgiving (no lighting/shadow/texture budget to fight),
  which matters directly for the multi-POV authoring cost flagged under
  Risks below — the same underlying capture can be re-colored and
  re-noised per POV and per layer without re-modeling geometry, which is
  what makes rendering one event several distorted ways actually
  affordable for a small team.
- **VR-specific payoff.** In the frozen, full-scale, walkable state, this
  aesthetic solves a problem VR walking-sims usually fight: dense, glowing
  points floating in true black *are* the environment, with no floor/wall
  collision fakery to hide, and up-close inspection (getting your face
  right next to a suspended point cluster) reads as a *feature* rather than
  a graphical seam the way it would in a rendered scene.

**Shape & scope.** Premium indie ($20–30), naturally episodic (one case =
one episode/DLC). Recommended v1 slice: **one case, three memories of one
event, three layers (visual / audio / emotional), screen-only.** Small
enough for a vertical slice in a few months, complete enough to prove
whether the subjective-memory mechanic is actually fun to solve. VR is a
post-validation port, not a launch requirement. Worth one line as a
far-future mode: asymmetric multiplayer where one player *lives and records*
a scenario and others investigate it afterward — user-generated mysteries.

**Risks.** Authoring cost of multi-POV scenes (build each event once, render
it per-POV with distortion deltas applied — a pipeline problem, not a design
problem, and solvable). Deduction difficulty tuning (Obra Dinn's
triple-confirmation pattern is the proven answer — don't accept a deduction
until three independent clues agree). IP care — see §8 below; the mechanic
is free to build, the branding is not.

## 5. Other ideas, in three tiers

**Tier 1 — substrate already exists, pain is acute, buildable now**

1. **Session braindance for AI coding agents.** Agent sessions already emit
   a complete event stream — prompts, tool calls, diffs, test runs. Product:
   scrubbable replay with layer isolation (reasoning / file edits / shell
   commands / test results), auto-placed markers where things went wrong,
   and frozen-moment free-roam that materializes the actual repo state at
   that instant in a throwaway worktree. Buyer: any team adopting agents who
   can't realistically review a 400-step transcript line by line. Incumbent
   is a flat chat log; this beats it by making the failure moment
   *walkable*. v1: a CLI that ingests a transcript plus git history and
   produces a local, scrubbable HTML timeline. Cheapest idea in the set to
   validate — it wins on substrate, though it's arguably the least
   "braindance-flavored" idea here since there's no subjectivity or
   sensory transfer involved.

2. **Retroactive incident braindance (ops/on-call).** Ring-buffer the last
   N minutes of what a responder sees — terminal, dashboards, chat, voice.
   When an incident is declared, the *pre-incident* window is already
   captured, no one had to remember to hit record. Layers: metrics /
   commands / comms; markers auto-generate the postmortem timeline.
   Incumbents reconstruct after the fact from disconnected logs; this is one
   scrubbable, multi-layer artifact of what the *human* actually did. The
   retention window and who can access it are the product, not an
   afterthought.

3. **Skill-transfer braindance (training).** Capture an expert's POV — video
   + tool telemetry + gaze — that a learner scrubs with layers isolated
   (hands / gaze / narration). What beats an ordinary tutorial video is
   gaze plus layer isolation, which turns passive watching into active
   study. Best wedge: a vertical where footage is already routinely
   recorded but never analyzed at this resolution (surgical residency,
   industrial apprenticeship, high-precision trades).

**Tier 2 — real ideas, heavier build or a more crowded field**

4. **Immersive first-person experience capture.** The literal in-lore BD
   industry, made real with today's spatial/immersive video and VR headsets:
   record performers, athletes, or extreme-sports participants in first
   person with a biometric overlay standing in for the "emotional layer"
   (heart rate, grip force). Sell the experience itself, not footage of it.
   This is the entertainment reading of braindance the flagship game and
   Tier 1 both skip past — worth calling out on its own.

5. **Personal life braindance / memex.** A layered, scrubbable personal
   timeline (mood, sleep, output, social contact, location) with an LLM
   maintaining running annotations. Differentiate from Rewind/Limitless-type
   products on genuine layer isolation and local-first storage, not just
   "we also record everything."

6. **Empathy braindance for user research.** Let the whole team
   re-experience one user's session — screen, affect signal, hesitation,
   think-aloud audio — instead of reading a research summary. Session
   replay tooling already exists broadly; almost nobody actually watches
   the recordings. Layers plus an LLM-picked "six moments that mattered" is
   the fix that might change that.

7. **Forensic braindance for security incident response.** Multi-layer
   overlay of one intrusion on a single scrub axis; freeze-frame
   reconstructs the compromised host in a sandbox for the analyst to walk
   through. Real value, but the competitive field (SOAR/XDR vendors) is
   mature and enterprise sales cycles are long.

8. **Sports & esports braindance.** Frozen-moment free-roam is exactly what
   analysts already wish they had. Esports is the cheap wedge — full game
   state is already recorded server-side, and comms/inputs/physiology are
   natural, ready-made layers.

**Tier 3 — literal reads of the game, most speculative**

9. **Braindance booth / venue installation.** 360° video, haptics, and
   thermal cues synced together — an art installation or brand activation,
   not really a repeatable product line.

10. **Consumer layered-capture wearable ("the wreath").** The
    picks-and-shovels play for all of the above. Also, notably, the
    consumer neurotech graveyard — Muse, Emotiv, Halo all tried variants of
    this and struggled to find a durable market.

11. **An open layered-experience file format + player.** The
    "MIDI-of-experience-capture" idea. Only valuable if something above
    actually takes off and needs a shared interchange format; treat this as
    a positioning note, not a product to build first.

## 6. What not to build

Another screen recorder wearing new branding. Anything gated on invasive
brain-computer interface hardware existing first. Anything that requires
consumer neurotech adoption as a precondition. A "BD editor" bolted on as a
feature of an existing analytics tool — the editor fantasy is strong enough
to deserve its own product (or, in the game's case, its own game), not a
menu item.

## 7. Ethics, consent & dosage

This isn't a disclaimer paragraph, it's a design requirement. First-person
capture of other people is the category's core liability across every idea
above: per-layer consent (someone may agree to visual capture but not
biometric), bounded retention, redaction of third parties who never
consented, and intensity/dosage limits on anything sold as an "experience"
rather than information. Building this in from v1 is a differentiator, not
a compliance tax — most competitors in these spaces treat it as an
afterthought.

The game gets its own note here: fiction lets *Recall* explore the harms the
real products above must actively prevent — memory as commodity, "hot BDs"
as addiction/overload. That thematic honesty is part of the game's pitch,
not just lore dressing.

## 8. IP & naming note

Game mechanics themselves aren't protectable — building this interaction
pattern, including the detective game, is legally fine. What to avoid is
CDPR's actual trade dress: don't ship a product or game called
"braindance," don't use Night City references, character names, or a
recreation of the in-game BD editor's specific UI. "Braindance" is used in
this document purely as shorthand for the pattern being described — every
product listed here needs its own name before it goes anywhere near a
storefront. This is a non-lawyer's observation; a trademark search in the
relevant classes is worth doing before any commercial launch.

## 9. Recommended next step

Two tracks worth pursuing in parallel, since they cost almost nothing to
validate against each other:

- **Product track — idea #1 (agent-session replay).** The capture substrate
  already exists for free (every agent session is already logged), the pain
  is new and growing fast, and a two-week v1 is realistic: parse a
  transcript, diff the repo at each tool call, render a scrubbable timeline.
  Honest caveat: it's the cheapest idea here to ship, but it's also the one
  that uses the least of what makes braindance interesting — no
  subjectivity, no sensory transfer, mostly the "scrub + freeze" primitives.

- **Game track — *Recall*.** The vertical-slice boundary is concrete: one
  case, three memories, three layers, screen-only. That's small enough to
  build and playtest in a few months, and it directly tests the one
  mechanic — subjective, contradictory memory — that nothing else on this
  list or on the market currently does. If it's fun in that slice, VR and
  further cases are a scoping decision, not a research risk.
