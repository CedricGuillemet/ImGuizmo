#pragma once

namespace ImSequencer
{

	enum SEQUENCER_OPTIONS
	{
		SEQUENCER_EDIT_NONE = 0,
		SEQUENCER_EDIT_STARTEND = 1 << 1,
		SEQUENCER_CHANGE_FRAME = 1 << 3,
		SEQUENCER_ADD = 1 << 4,
		SEQUENCER_DEL = 1 << 5,
		SEQUENCER_COPYPASTE = 1 << 6,
		SEQUENCER_EDIT_ALL = SEQUENCER_EDIT_STARTEND | SEQUENCER_CHANGE_FRAME
	};

	struct SequenceInterface
	{
		virtual int GetFrameCount() const = 0;
		virtual int GetItemCount() const = 0;

		virtual int GetItemTypeCount() const { return 0; }
		virtual const char *GetItemTypeName(int /*typeIndex*/) const { return ""; }
		virtual const char *GetItemLabel(int /*index*/) const { return ""; }

		virtual void Get(int index, int** start, int** end, int *type, unsigned int *color) = 0;
		virtual void Add(int /*type*/) {}
		virtual void Del(int /*index*/) {}
		virtual void Duplicate(int /*index*/) {}

		virtual void Copy() {}
		virtual void Paste() {}
	};


	// return true if selection is made
	bool Sequencer(SequenceInterface *sequence, int *currentFrame, bool *expanded, int *selectedEntry, int *firstFrame, int sequenceOptions);

}