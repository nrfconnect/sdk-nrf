# Vale Documentation Linting

This directory contains configuration files for [Vale](https://vale.sh/docs), an open-source documentation linting tool.

## Setup

1. [Install Vale](https://vale.sh/docs/install):
   - **Windows**: `choco install vale`
   - **macOS**: `brew install vale`
   - **Linux**: `snap install vale`

2. Run the linting:
   - **Windows**: `.\scripts\check-docs.ps1`
   - **Linux/macOS**: `./scripts/check-docs.sh`

## Configuration

- `.vale.ini`: Main configuration file (located in the repository root)
- `.github/vale/styles/`: Contains custom style rules
- `.github/vale/vocab/`: Contains project-specific vocabulary

## CI Integration

Vale automatically runs on pull requests that modify documentation files via the GitHub Actions workflow defined in `.github/workflows/vale-lint.yml`.

## Adding New Rules

To add new style rules:

1. Create a new YAML file in `.github/vale/styles/Vale/`.
2. Define the rule using Vale's [style syntax](https://vale.sh/docs/styles).
3. Test the rule locally.

## Project-Specific Vocabulary

Add project-specific terms to:
- `.github/vale/vocab/Base/accept.txt` for words that should be accepted
- `.github/vale/vocab/Base/reject.txt` for words that should be flagged