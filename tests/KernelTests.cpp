/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Tests for PatternKernel, KernelPresets, and related data structures.
 */

#include "core/GrooveboxProject.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace SurgeBox;

static int testsRun = 0;
static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name)                         \
    do                                     \
    {                                      \
        testsRun++;                        \
        std::cout << "  " << name << "..."; \
    } while (0)

#define PASS()                            \
    do                                    \
    {                                     \
        testsPassed++;                    \
        std::cout << " PASS" << std::endl; \
    } while (0)

#define FAIL(msg)                                                       \
    do                                                                  \
    {                                                                   \
        testsFailed++;                                                  \
        std::cout << " FAIL: " << msg << " (line " << __LINE__ << ")" \
                  << std::endl;                                         \
    } while (0)

#define ASSERT_EQ(a, b)                                                       \
    do                                                                        \
    {                                                                         \
        if ((a) != (b))                                                       \
        {                                                                     \
            FAIL(#a " != " #b);                                               \
            return;                                                           \
        }                                                                     \
    } while (0)

#define ASSERT_TRUE(expr)                                               \
    do                                                                  \
    {                                                                   \
        if (!(expr))                                                    \
        {                                                               \
            FAIL(#expr " is false");                                    \
            return;                                                     \
        }                                                               \
    } while (0)

#define ASSERT_FALSE(expr)                                              \
    do                                                                  \
    {                                                                   \
        if ((expr))                                                     \
        {                                                               \
            FAIL(#expr " is true");                                     \
            return;                                                     \
        }                                                               \
    } while (0)

#define ASSERT_NEAR(a, b, tol)                                          \
    do                                                                  \
    {                                                                   \
        if (std::fabs((a) - (b)) > (tol))                               \
        {                                                               \
            FAIL(#a " not near " #b);                                   \
            return;                                                     \
        }                                                               \
    } while (0)

// ============================================================================
// KernelCell Tests
// ============================================================================

void testKernelCellDefaults()
{
    TEST("KernelCell default construction");
    KernelCell cell;
    ASSERT_EQ(cell.pitchOffset, 0);
    ASSERT_NEAR(cell.timeOffset, 0.0, 0.001);
    ASSERT_NEAR(cell.velocityScale, 1.0f, 0.001f);
    ASSERT_NEAR(cell.probability, 1.0f, 0.001f);
    PASS();
}

void testKernelCellAggregateInit()
{
    TEST("KernelCell aggregate initialization");
    KernelCell cell{7, 0.5, 0.8f, 0.6f};
    ASSERT_EQ(cell.pitchOffset, 7);
    ASSERT_NEAR(cell.timeOffset, 0.5, 0.001);
    ASSERT_NEAR(cell.velocityScale, 0.8f, 0.001f);
    ASSERT_NEAR(cell.probability, 0.6f, 0.001f);
    PASS();
}

// ============================================================================
// PatternKernel Tests
// ============================================================================

void testPatternKernelDefaults()
{
    TEST("PatternKernel default construction");
    PatternKernel k;
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Off));
    ASSERT_TRUE(k.cells.empty());
    ASSERT_TRUE(k.scaleAware);
    ASSERT_EQ(k.scaleRoot, 0);
    ASSERT_EQ(k.scaleType, 0);
    ASSERT_EQ(k.pivotPitch, 60);
    ASSERT_EQ(k.accumulateSemitones, 0);
    ASSERT_EQ(k.resetAfterIterations, 0);
    ASSERT_EQ(k.seed, 0u);
    ASSERT_FALSE(k.followChords);
    PASS();
}

void testPatternKernelIsActive()
{
    TEST("PatternKernel::isActive()");
    PatternKernel k;

    // Off mode is inactive
    ASSERT_FALSE(k.isActive());

    // Spawn mode with no cells is still inactive
    k.mode = KernelMode::Spawn;
    ASSERT_FALSE(k.isActive());

    // Spawn mode with cells is active
    k.cells.push_back({0, 0.0, 1.0f, 1.0f});
    ASSERT_TRUE(k.isActive());

    // Transform mode with cells is active
    k.mode = KernelMode::Transform;
    ASSERT_TRUE(k.isActive());

    // Invert mode
    k.mode = KernelMode::Invert;
    ASSERT_TRUE(k.isActive());

    // Retrograde mode
    k.mode = KernelMode::Retrograde;
    ASSERT_TRUE(k.isActive());

    // Off mode with cells is inactive
    k.mode = KernelMode::Off;
    ASSERT_FALSE(k.isActive());

    PASS();
}

void testPatternKernelModeValues()
{
    TEST("KernelMode enum values");
    ASSERT_EQ(static_cast<int>(KernelMode::Off), 0);
    ASSERT_EQ(static_cast<int>(KernelMode::Spawn), 1);
    ASSERT_EQ(static_cast<int>(KernelMode::Transform), 2);
    ASSERT_EQ(static_cast<int>(KernelMode::Invert), 3);
    ASSERT_EQ(static_cast<int>(KernelMode::Retrograde), 4);
    PASS();
}

// ============================================================================
// Kernel Preset Tests
// ============================================================================

void testPresetArpeggioUp()
{
    TEST("KernelPresets::arpeggioUp()");
    auto k = KernelPresets::arpeggioUp();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.scaleAware);
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 2);

    // First cell should be identity (pitch 0, time 0)
    ASSERT_EQ(k.cells[0].pitchOffset, 0);
    ASSERT_NEAR(k.cells[0].timeOffset, 0.0, 0.001);
    ASSERT_NEAR(k.cells[0].velocityScale, 1.0f, 0.001f);

    // Subsequent cells should have higher pitches
    for (size_t i = 1; i < k.cells.size(); i++)
    {
        ASSERT_TRUE(k.cells[i].pitchOffset > 0);
        ASSERT_TRUE(k.cells[i].timeOffset > k.cells[i - 1].timeOffset);
    }
    PASS();
}

void testPresetArpeggioDown()
{
    TEST("KernelPresets::arpeggioDown()");
    auto k = KernelPresets::arpeggioDown();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 2);

    // Descends: first cell has highest pitch offset, last has lowest
    for (size_t i = 1; i < k.cells.size(); i++)
        ASSERT_TRUE(k.cells[i].pitchOffset <= k.cells[i - 1].pitchOffset);

    PASS();
}

void testPresetArpeggioUpDown()
{
    TEST("KernelPresets::arpeggioUpDown()");
    auto k = KernelPresets::arpeggioUpDown();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 3);
    PASS();
}

void testPresetChord()
{
    TEST("KernelPresets::chord()");
    auto k = KernelPresets::chord();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 2);

    // Chord: all cells at time offset 0 (simultaneous)
    for (const auto &cell : k.cells)
        ASSERT_NEAR(cell.timeOffset, 0.0, 0.001);

    PASS();
}

void testPresetOctaveDouble()
{
    TEST("KernelPresets::octaveDouble()");
    auto k = KernelPresets::octaveDouble();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());

    // Should have a cell with +12 semitones (octave)
    bool hasOctave = false;
    for (const auto &cell : k.cells)
    {
        if (cell.pitchOffset == 12)
            hasOctave = true;
    }
    ASSERT_TRUE(hasOctave);
    PASS();
}

void testPresetEcho()
{
    TEST("KernelPresets::echo()");
    auto k = KernelPresets::echo();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 2);

    // Echo: same pitch, different time offsets with decaying velocity
    for (size_t i = 1; i < k.cells.size(); i++)
    {
        ASSERT_TRUE(k.cells[i].timeOffset > 0.0);
        ASSERT_TRUE(k.cells[i].velocityScale < k.cells[0].velocityScale);
    }
    PASS();
}

void testPresetStrum()
{
    TEST("KernelPresets::strum()");
    auto k = KernelPresets::strum();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Spawn));
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.cells.size() >= 2);

    // Strum: small time offsets, different pitches
    for (const auto &cell : k.cells)
        ASSERT_TRUE(std::fabs(cell.timeOffset) < 1.0);

    PASS();
}

void testPresetProbabilityThin()
{
    TEST("KernelPresets::probabilityThin()");
    auto k = KernelPresets::probabilityThin();
    ASSERT_TRUE(k.isActive());

    // Should have at least one cell with probability < 1.0
    bool hasProbability = false;
    for (const auto &cell : k.cells)
    {
        if (cell.probability < 1.0f)
            hasProbability = true;
    }
    ASSERT_TRUE(hasProbability);
    PASS();
}

void testPresetRisingSequence()
{
    TEST("KernelPresets::risingSequence()");
    auto k = KernelPresets::risingSequence();
    ASSERT_TRUE(k.isActive());
    ASSERT_TRUE(k.accumulateSemitones > 0);
    PASS();
}

void testPresetInvertMelody()
{
    TEST("KernelPresets::invertMelody()");
    auto k = KernelPresets::invertMelody();
    ASSERT_EQ(static_cast<int>(k.mode), static_cast<int>(KernelMode::Invert));
    ASSERT_TRUE(k.isActive());
    ASSERT_EQ(k.pivotPitch, 60);

    // Custom pivot
    auto k2 = KernelPresets::invertMelody(72);
    ASSERT_EQ(k2.pivotPitch, 72);
    PASS();
}

void testAllPresetsHaveValidCells()
{
    TEST("All presets have valid cells");
    auto presets = {
        KernelPresets::arpeggioUp(),
        KernelPresets::arpeggioDown(),
        KernelPresets::arpeggioUpDown(),
        KernelPresets::chord(),
        KernelPresets::octaveDouble(),
        KernelPresets::echo(),
        KernelPresets::strum(),
        KernelPresets::probabilityThin(),
        KernelPresets::risingSequence(),
        KernelPresets::invertMelody()};

    for (const auto &p : presets)
    {
        ASSERT_TRUE(p.isActive());
        for (const auto &cell : p.cells)
        {
            ASSERT_TRUE(cell.pitchOffset >= -48 && cell.pitchOffset <= 48);
            ASSERT_TRUE(cell.timeOffset >= -4.0 && cell.timeOffset <= 4.0);
            ASSERT_TRUE(cell.velocityScale >= 0.0f && cell.velocityScale <= 1.0f);
            ASSERT_TRUE(cell.probability >= 0.0f && cell.probability <= 1.0f);
        }
    }
    PASS();
}

// ============================================================================
// Pattern struct tests
// ============================================================================

void testPatternDefaults()
{
    TEST("Pattern default construction");
    Pattern p;
    ASSERT_TRUE(p.notes.empty());
    ASSERT_EQ(p.bars, 4);
    ASSERT_NEAR(p.swing, 0.0, 0.001);
    ASSERT_TRUE(p.loopRegions.empty());
    ASSERT_FALSE(p.kernel.isActive());
    ASSERT_FALSE(p.snapToChord);
    ASSERT_NEAR(p.lengthInBeats(), 16.0, 0.001);
    PASS();
}

void testPatternAddRemoveNotes()
{
    TEST("Pattern add/remove notes");
    Pattern p;
    p.addNote(0.0, 0.25, 60, 100);
    p.addNote(1.0, 0.5, 64, 80);
    ASSERT_EQ(p.notes.size(), 2u);

    p.removeNote(0);
    ASSERT_EQ(p.notes.size(), 1u);
    ASSERT_EQ(p.notes[0].pitch, 64);
    PASS();
}

void testPatternKernelAssignment()
{
    TEST("Pattern kernel assignment");
    Pattern p;
    auto k = KernelPresets::arpeggioUp();
    p.kernel = k;
    ASSERT_TRUE(p.kernel.isActive());
    ASSERT_EQ(static_cast<int>(p.kernel.mode), static_cast<int>(KernelMode::Spawn));
    PASS();
}

void testPatternSnapToChord()
{
    TEST("Pattern snapToChord flag");
    Pattern p;
    ASSERT_FALSE(p.snapToChord);
    p.snapToChord = true;
    ASSERT_TRUE(p.snapToChord);
    PASS();
}

// ============================================================================
// MIDINote tests
// ============================================================================

void testMIDINoteDefaults()
{
    TEST("MIDINote default construction");
    MIDINote n;
    ASSERT_NEAR(n.startBeat, 0.0, 0.001);
    ASSERT_NEAR(n.duration, 1.0, 0.001);
    ASSERT_EQ(n.pitch, 60);
    ASSERT_EQ(n.velocity, 100);
    ASSERT_NEAR(n.endBeat(), 1.0, 0.001);
    PASS();
}

void testMIDINoteConstructor()
{
    TEST("MIDINote parameterized construction");
    MIDINote n(2.5, 0.5, 72, 127);
    ASSERT_NEAR(n.startBeat, 2.5, 0.001);
    ASSERT_NEAR(n.duration, 0.5, 0.001);
    ASSERT_EQ(n.pitch, 72);
    ASSERT_EQ(n.velocity, 127);
    ASSERT_NEAR(n.endBeat(), 3.0, 0.001);
    PASS();
}

// ============================================================================
// LoopRegion tests
// ============================================================================

void testLoopRegionDefaults()
{
    TEST("LoopRegion default construction");
    LoopRegion lr;
    ASSERT_NEAR(lr.startBeat, 0.0, 0.001);
    ASSERT_NEAR(lr.endBeat, 0.0, 0.001);
    ASSERT_EQ(lr.minPitch, 0);
    ASSERT_EQ(lr.maxPitch, 127);
    ASSERT_FALSE(lr.active);
    ASSERT_NEAR(lr.length(), 0.0, 0.001);
    PASS();
}

void testLoopRegionContainsPitch()
{
    TEST("LoopRegion::containsPitch()");
    LoopRegion lr;
    lr.minPitch = 48;
    lr.maxPitch = 72;

    ASSERT_TRUE(lr.containsPitch(60));
    ASSERT_TRUE(lr.containsPitch(48));
    ASSERT_TRUE(lr.containsPitch(72));
    ASSERT_FALSE(lr.containsPitch(47));
    ASSERT_FALSE(lr.containsPitch(73));
    PASS();
}

// ============================================================================
// VoiceSelector index mapping tests (data-level)
// ============================================================================

void testVoiceSelectorIndices()
{
    TEST("VoiceSelector index mapping (K=4, CH=5, FX=6)");
    // The VoiceSelector has 7 buttons: 1,2,3,4,K,CH,FX
    // Indices: 0,1,2,3,4,5,6
    // K = NUM_VOICES (4), CH = NUM_VOICES+1 (5), FX = NUM_VOICES+2 (6)
    ASSERT_EQ(NUM_VOICES, 4);
    ASSERT_EQ(NUM_VOICES + 0, 4);  // K index
    ASSERT_EQ(NUM_VOICES + 1, 5);  // CH index
    ASSERT_EQ(NUM_VOICES + 2, 6);  // FX index
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "=== SurgeBox Kernel Tests ===" << std::endl;

    std::cout << std::endl
              << "[KernelCell]" << std::endl;
    testKernelCellDefaults();
    testKernelCellAggregateInit();

    std::cout << std::endl
              << "[PatternKernel]" << std::endl;
    testPatternKernelDefaults();
    testPatternKernelIsActive();
    testPatternKernelModeValues();

    std::cout << std::endl
              << "[KernelPresets]" << std::endl;
    testPresetArpeggioUp();
    testPresetArpeggioDown();
    testPresetArpeggioUpDown();
    testPresetChord();
    testPresetOctaveDouble();
    testPresetEcho();
    testPresetStrum();
    testPresetProbabilityThin();
    testPresetRisingSequence();
    testPresetInvertMelody();
    testAllPresetsHaveValidCells();

    std::cout << std::endl
              << "[Pattern]" << std::endl;
    testPatternDefaults();
    testPatternAddRemoveNotes();
    testPatternKernelAssignment();
    testPatternSnapToChord();

    std::cout << std::endl
              << "[MIDINote]" << std::endl;
    testMIDINoteDefaults();
    testMIDINoteConstructor();

    std::cout << std::endl
              << "[LoopRegion]" << std::endl;
    testLoopRegionDefaults();
    testLoopRegionContainsPitch();

    std::cout << std::endl
              << "[VoiceSelector]" << std::endl;
    testVoiceSelectorIndices();

    std::cout << std::endl
              << "=== Results: " << testsPassed << "/" << testsRun << " passed";
    if (testsFailed > 0)
        std::cout << " (" << testsFailed << " FAILED)";
    std::cout << " ===" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
