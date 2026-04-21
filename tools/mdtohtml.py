#!/usr/bin/env python3
import argparse
import re
import markdown
from pathlib import Path

try:
    from markdown.extensions.codehilite import CodeHiliteExtension
    from pygments.formatters import HtmlFormatter
    pygments_available = True
except ImportError:
    print("Pygments not found. Code highlighting will be disabled.")
    pygments_available = False

def main():
    parser = argparse.ArgumentParser(description="Convert Markdown to HTML with syntax highlighting.")
    parser.add_argument("input", help="Input Markdown file")
    parser.add_argument("output", nargs="?", default=None, help="Output HTML file (default: input.html)")
    parser.add_argument("--css", action="store_true", help="Include inline CSS for basic styling")
    parser.add_argument("--style", default="tango", help="Pygments style to use for highlighting (default: tango)")
    args = parser.parse_args()

    # Read input file
    with open(args.input, "r", encoding="utf-8") as f:
        text = f.read()

    # Configure Markdown
    extensions = ['tables', 'fenced_code']
    if pygments_available:
        # Determine available styles and pick a valid one (fall back if requested style is missing)
        try:
            from pygments.styles import get_all_styles
            available_styles = set(get_all_styles())
        except Exception:
            available_styles = set()

        requested_style = args.style or "tango"
        final_style = requested_style
        if available_styles and requested_style not in available_styles:
            # try a few sensible fallbacks
            fallbacks = [requested_style + '-light', requested_style + '-dark', 'github-dark', 'default', 'xcode', 'solarized-light', 'tango']
            for s in fallbacks:
                if s in available_styles:
                    final_style = s
                    break
            else:
                final_style = 'default'
            print(f"Warning: requested Pygments style '{requested_style}' not found; using '{final_style}' instead.")

        extensions.append(CodeHiliteExtension(linenums=None, pygments_style=final_style, css_class="highlight", noclasses=True))
    md = markdown.Markdown(extensions=extensions)

    # Convert
    html = md.convert(text)

    # Apply inline styles to markdown tables only, without touching code block markup.
    def _style_table_block(table_match):
        table_html = table_match.group(0)

        # Keep non-markdown tables (e.g., possible syntax-highlighter internals) unchanged.
        if '<thead>' not in table_html or '<tbody>' not in table_html:
            return table_html

        table_html = re.sub(
            r'<table(.*?)>',
            '<table\\1 style="width:100%; border-collapse:collapse; margin:1.2rem 0; border:1px solid #d0d7de; border-radius:8px; overflow:hidden; box-shadow:0 1px 2px rgba(0,0,0,0.04);">',
            table_html,
            count=1,
        )
        table_html = table_html.replace('<thead>', '<thead style="background:#f6f8fa; color:#24292f;">')
        table_html = table_html.replace('<th>', '<th style="text-align:left; font-weight:600; border-bottom:2px solid #d0d7de; padding:0.65rem 0.8rem; vertical-align:top;">')
        table_html = table_html.replace('<td>', '<td style="padding:0.65rem 0.8rem; border-bottom:1px solid #eaeef2; vertical-align:top;">')

        # Zebra-strip tbody rows with alternating backgrounds.
        def _style_tbody(tbody_match):
            row_idx = {'n': 0}

            def _style_tr(match):
                row_idx['n'] += 1
                bg = '#fbfcfd' if row_idx['n'] % 2 == 0 else '#ffffff'
                return f'<tr style="background:{bg};">'

            tbody = tbody_match.group(0)
            return re.sub(r'<tr>', _style_tr, tbody)

        return re.sub(r'<tbody>.*?</tbody>', _style_tbody, table_html, flags=re.DOTALL)

    html = re.sub(r'<table\b[^>]*>.*?</table>', _style_table_block, html, flags=re.DOTALL)

    # Add basic CSS if requested
    if args.css:
        css = """
        <style>
            body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
            .highlight { background: #f8f8f8; border: 1px solid #ddd; padding: 10px; border-radius: 4px; }
            pre { margin: 0; }
        </style>
        """
        html = css + html

    # Write output
    output_file = args.output or Path(args.input).with_suffix(".html")
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>{Path(args.input).stem}</title>
</head>
<body>
    {html}
</body>
</html>""")

    print(f"Converted {args.input} to {output_file}")

if __name__ == "__main__":
    main()
