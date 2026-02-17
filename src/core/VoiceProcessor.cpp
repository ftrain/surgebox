/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "VoiceProcessor.h"
#include "PlaceholderProcessor.h"

namespace SurgeBox
{

bool VoiceProcessor::isPlaceholder() const
{
    return dynamic_cast<PlaceholderProcessor *>(processor_.get()) != nullptr;
}

} // namespace SurgeBox
