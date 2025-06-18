#!/bin/bash
# git-auto-push.sh
# Usage: ./git-auto-push.sh "<tag string>"
# Stages all changes, commits with the provided tag string, and pushes to the current branch.

TAG_STRING="$1"

if [ -z "$TAG_STRING" ]; then
  echo "Usage: $0 \"<tag string>\""
  exit 1
fi

# Stage all changes
 git add .

# Commit with the tag string as the message
 git commit -m "$TAG_STRING"

# Push to the current branch
 git push 