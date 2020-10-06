# Earley Pattern Language #

# Bound Patterns #

The pattern that is bound obeys the following rules:

1. It must contain no binders itself.
2. It must not be a SeqPat.
2. If it contains a production, it must be of the right form.
  If ListPat: Pattern must be of the right-form.
  If Repeat: Pattern must be of the right form.
  ChoicePat: All patterns must of the right form.
  Production: Is the right form.
  No other pattern is of the right form.
3. If it contains no productions, then it's okay.
