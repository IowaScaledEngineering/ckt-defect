#!/usr/bin/env python3
"""
Generate a publication-grade PDF specification of the Message Token Modifiers
and Audio Parsing Architecture for the ckt-defect Defect Detector firmware.
"""

import os
import sys
import shutil
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            canvas.Canvas.showPage(self)
        canvas.Canvas.save(self)

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#718096"))
        
        # Header (pages after page 1)
        if self._pageNumber > 1:
            self.drawString(54, 752, "Defect Detector | Message Token Modifiers & Audio Parsing Specification")
            self.setStrokeColor(colors.HexColor("#CBD5E0"))
            self.setLineWidth(0.5)
            self.line(54, 746, 558, 746)

        # Footer (all pages)
        self.setStrokeColor(colors.HexColor("#CBD5E0"))
        self.setLineWidth(0.5)
        self.line(54, 45, 558, 45)
        
        self.drawString(54, 32, "Confidential - Engineering Architectural Specification")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(558, 32, page_str)
        self.restoreState()

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PDF = os.path.join(SCRIPT_DIR, "token_modifiers_reference.pdf")

def build_pdf(filename=DEFAULT_PDF):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=46,
        bottomMargin=48
    )

    styles = getSampleStyleSheet()

    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=17,
        leading=21,
        textColor=colors.HexColor("#1A365D"),
        spaceAfter=3
    )

    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=13,
        textColor=colors.HexColor("#2B6CB0"),
        spaceAfter=6
    )

    meta_style = ParagraphStyle(
        'DocMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor("#4A5568")
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=11,
        leading=14,
        textColor=colors.HexColor("#1A365D"),
        spaceBefore=7,
        spaceAfter=3,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9,
        leading=12,
        textColor=colors.HexColor("#2B6CB0"),
        spaceBefore=5,
        spaceAfter=2,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor("#2D3748"),
        spaceAfter=3
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=7,
        leading=9,
        textColor=colors.white,
        alignment=1 # Center
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=6.8,
        leading=8.8,
        textColor=colors.HexColor("#2D3748")
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=6.8,
        leading=8.8,
        textColor=colors.HexColor("#1A365D")
    )

    table_cell_code = ParagraphStyle(
        'TableCellCode',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=6.5,
        leading=8.5,
        textColor=colors.HexColor("#1A202C")
    )

    test_cell_style = ParagraphStyle(
        'TestCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=6.2,
        leading=8,
        textColor=colors.HexColor("#2D3748")
    )

    test_cell_bold = ParagraphStyle(
        'TestCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=6.2,
        leading=8,
        textColor=colors.HexColor("#1A365D")
    )

    test_cell_code = ParagraphStyle(
        'TestCellCode',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=6.0,
        leading=7.8,
        textColor=colors.HexColor("#1A202C")
    )

    callout_style = ParagraphStyle(
        'CalloutText',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.6,
        leading=10.4,
        textColor=colors.HexColor("#1A365D")
    )

    elements = []

    # ==================== PAGE 1: TITLE & ARCHITECTURE ====================
    elements.append(Paragraph("Message Token Modifiers & Audio Parsing Specification", title_style))
    elements.append(Paragraph("Two-Stage Pipeline Architecture, Formatting Modifiers, and Playback Directives", subtitle_style))

    meta_text = "<b>Firmware:</b> ESP32-S2 (<code>ckt-defect</code>) | <b>Source:</b> <code>messages.cpp</code>, <code>parser.cpp</code> | <b>Test Suite:</b> <code>test/messages</code>"
    elements.append(Paragraph(meta_text, meta_style))
    elements.append(Spacer(1, 3))
    elements.append(HRFlowable(width="100%", thickness=1.2, color=colors.HexColor("#1A365D"), spaceAfter=5))

    elements.append(Paragraph("1. System Architecture & Dual-Mode Execution Model", h1_style))
    elements.append(Paragraph(
        "Message generation in the defect detector firmware operates as a coordinated two-stage pipeline. "
        "<b>Stage 1</b> (<code>transformMessage</code>) expands variable placeholders into concrete text and applies column formatting. "
        "<b>Stage 2</b> (<code>parseTask</code>) runs as a dedicated FreeRTOS task, converting the expanded token stream into real-time spoken audio and synthetic waveforms.",
        body_style
    ))

    # Architecture Box
    callout_data = [
        [
            Paragraph(
                "• <b>Display Mode (<code>breakDigits = false</code>):</b> Formats text for the 20x4 LCD screen. "
                "Preserves numbers as contiguous values (e.g. <code>\"124\"</code>), enforces exact field padding/alignment constraints, "
                "and converts single backslashes (<code>\\</code>) into newlines (<code>\\n</code>) to split rows.<br/>"
                "• <b>Spoken Mode (<code>breakDigits = true</code>):</b> Prepares text for the voice synthesizer. "
                "Splits numbers into individual whitespace-separated digits and tokens (e.g. <code>\"1 2 4\"</code>, <code>\"3 4 6 . 9\"</code>) "
                "so the audio engine can perform word-by-word vocabulary lookups. Integer padding constraints are bypassed.<br/>"
                "• <b>Pass-Through Directives:</b> Audio generator tokens (<code>#tone</code>, <code>#pause</code>, <code>#count</code>, <code>#ord</code>) "
                "are untouched by Stage 1 and executed dynamically by Stage 2.",
                callout_style
            )
        ]
    ]
    t_callout = Table(callout_data, colWidths=[504])
    t_callout.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EBF8FF")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#3182CE")),
        ('PADDING', (0,0), (-1,-1), 4),
    ]))
    elements.append(t_callout)
    elements.append(Spacer(1, 5))

    # Section 2: Stage 1 Specification
    elements.append(Paragraph("2. Stage 1: <code>transformMessage</code> Reference", h1_style))
    elements.append(Paragraph(
        "<b>Syntax:</b> <code>#&lt;baseToken&gt;[:[±]N[.M]]</code> — <code>:</code> separates the modifier. "
        "<code>±</code> selects alignment (omitted/positive = right-aligned / left space padded; <code>-</code> = left-aligned / right space padded). "
        "<code>N</code> is field width; <code>.M</code> specifies fractional digits (for <code>#milepost</code>).",
        body_style
    ))

    s1_data = [
        [
            Paragraph("Token", table_header_style),
            Paragraph("Modifier Syntax", table_header_style),
            Paragraph("Value Source & Type", table_header_style),
            Paragraph("Display Mode (<code>breakDigits=false</code>)", table_header_style),
            Paragraph("Spoken Mode (<code>breakDigits=true</code>)", table_header_style)
        ],
        [
            Paragraph("<b><code>\\</code></b>", table_cell_bold),
            Paragraph("None", table_cell_style),
            Paragraph("Control Character", table_cell_style),
            Paragraph("Converted to newline (<code>\\n</code>) for LCD lines", table_cell_style),
            Paragraph("Converted to newline (<code>\\n</code>)", table_cell_style)
        ],
        [
            Paragraph("<b><code>\\\\</code></b>", table_cell_bold),
            Paragraph("None", table_cell_style),
            Paragraph("Escape Sequence", table_cell_style),
            Paragraph("Converted to literal backslash (<code>\\</code>)", table_cell_style),
            Paragraph("Converted to literal backslash (<code>\\</code>)", table_cell_style)
        ],
        [
            Paragraph("<b><code>#milepost</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N[.M]</code><br/><i>(def: 1.1)</i>", table_cell_code),
            Paragraph("<code>cfg.milepost</code><br/>(tenths of a mile, int)", table_cell_style),
            Paragraph("<code>#milepost:4.1</code> with 3469 $\\to$ <code>\" 346.9\"</code><br/><code>#milepost:-4.1</code> $\\to$ <code>\"346.9 \"</code>", table_cell_code),
            Paragraph("Padding ignored; digits split:<br/><code>\"3 4 6 . 9\"</code>", table_cell_code)
        ],
        [
            Paragraph("<b><code>#track</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(optional)</i>", table_cell_code),
            Paragraph("<code>cfg.trackName[trackNum]</code><br/>(string)", table_cell_style),
            Paragraph("Truncates to <code>|N|</code> if longer; pads left (<code>N&gt;0</code>) or right (<code>N&lt;0</code>). <code>#track:4</code> $\\to$ <code>\"Main\"</code>", table_cell_style),
            Paragraph("Truncates or pads string field verbatim", table_cell_style)
        ],
        [
            Paragraph("<b><code>#direction</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(optional)</i>", table_cell_code),
            Paragraph("<code>data.direction</code><br/>(1=dir1, 2=dir2, other=empty)", table_cell_style),
            Paragraph("Truncates or pads direction name. Returns empty string <code>\"\"</code> if direction is unknown", table_cell_style),
            Paragraph("Truncates or pads direction string", table_cell_style)
        ],
        [
            Paragraph("<b><code>#rail</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(optional)</i>", table_cell_code),
            Paragraph("<code>cfg.railName</code> (e.g. <code>\"North/South\"</code>)<br/>& <code>data.rail</code>", table_cell_style),
            Paragraph("Splits on <code>'/'</code>: odd rail selects 1st half (<code>\"North\"</code>), even selects 2nd (<code>\"South\"</code>). Truncates/pads.", table_cell_style),
            Paragraph("Selects rail side; truncates/pads", table_cell_style)
        ],
        [
            Paragraph("<b><code>#axle</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(def: 1)</i>", table_cell_code),
            Paragraph("<code>data.axleCount</code><br/>(defect location, int)", table_cell_style),
            Paragraph("<code>#axle:3</code> with 4 $\\to$ <code>\"  4\"</code> (right-aligned)<br/><code>#axle:-3</code> $\\to$ <code>\"4  \"</code> (left-aligned)", table_cell_code),
            Paragraph("Emitted as <code>\"4\"</code>", table_cell_code)
        ],
        [
            Paragraph("<b><code>#axles</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(def: 1)</i>", table_cell_code),
            Paragraph("<code>data.totalAxles</code><br/>(total axles, int)", table_cell_style),
            Paragraph("<code>#axles:5</code> with 124 $\\to$ <code>\"  124\"</code><br/><code>#axles:-5</code> $\\to$ <code>\"124  \"</code>", table_cell_code),
            Paragraph("Padding ignored; digits split:<br/><code>\"1 2 4\"</code>", table_cell_code)
        ],
        [
            Paragraph("<b><code>#speed</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(def: 1)</i>", table_cell_code),
            Paragraph("<code>data.speed</code><br/>(train speed mph, int)", table_cell_style),
            Paragraph("<code>#speed:3</code> with 45 $\\to$ <code>\" 45\"</code><br/><code>#speed:-3</code> $\\to$ <code>\"45 \"</code>", table_cell_code),
            Paragraph("Padding ignored; digits split:<br/><code>\"4 5\"</code>", table_cell_code)
        ],
        [
            Paragraph("<b><code>#temp</code></b>", table_cell_bold),
            Paragraph("<code>:[±]N</code><br/><i>(def: 1)</i>", table_cell_code),
            Paragraph("<code>TemperatureManager</code><br/>(ambient temp + 0.5, int)", table_cell_style),
            Paragraph("<code>#temp:3</code> with 72 $\\to$ <code>\" 72\"</code>", table_cell_code),
            Paragraph("Padding ignored; digits split:<br/><code>\"7 2\"</code>", table_cell_code)
        ],
        [
            Paragraph("<b><code>#defectlist</code></b>", table_cell_bold),
            Paragraph("None", table_cell_style),
            Paragraph("<code>data.defects</code><br/>(vector of pre-transformed strings)", table_cell_style),
            Paragraph("Concatenates all defect details with single space separator: e.g. <code>\"HOT_BOX DRAG_EQ\"</code>", table_cell_style),
            Paragraph("Concatenates detail messages with single space separator", table_cell_style)
        ]
    ]

    t_s1 = Table(s1_data, colWidths=[65, 55, 110, 137, 137])
    t_s1.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_s1)

    # ==================== PAGE 2: STAGE 2 AUDIO & WORKFLOWS ====================
    elements.append(PageBreak())

    elements.append(Paragraph("3. Stage 2: <code>parseTask</code> Audio Directives & Speech Synthesis", h1_style))
    elements.append(Paragraph(
        "<code>parseTask</code> executes on a dedicated FreeRTOS task, popping <code>ParserObject</code> items from <code>parserQueue</code>. "
        "It updates the LCD display message (protected by <code>displayMutex</code>) and evaluates the spoken message token by token (whitespace delimited).",
        body_style
    ))

    s2_data = [
        [
            Paragraph("Directive / Token", table_header_style),
            Paragraph("Syntax & Defaults", table_header_style),
            Paragraph("Synthesis Mechanism & Mathematical Formula", table_header_style),
            Paragraph("Examples & Behavior", table_header_style)
        ],
        [
            Paragraph("<b><code>#tone</code></b><br/>Tone Generator", table_cell_bold),
            Paragraph("<code>#tone</code><br/><code>#tone=M</code><br/><code>#tone=M,N</code><br/><i>(def: M=10, N=1)</i>", table_cell_code),
            Paragraph(
                "Generates synthetic 1,000 Hz sine wave tone in RAM (<code>ToneSound</code>) at 16 kHz.<br/>"
                "• <b><code>M</code>:</b> Duration in deciseconds (10 = 1.0 s; samples = $1600 \\times M$).<br/>"
                "• <b><code>N</code>:</b> Volume attenuation exponent: $\\text{Amplitude} = 1 / 2^N$.<br/>"
                "&nbsp;&nbsp;• $N=0 \\to 100\\%$ (0 dB) &nbsp;&nbsp;• $N=1 \\to 50\\%$ (-6 dB)<br/>"
                "&nbsp;&nbsp;• $N=2 \\to 25\\%$ (-12 dB) &nbsp;• $N=3 \\to 12.5\\%$ (-18 dB)",
                table_cell_style
            ),
            Paragraph(
                "• <code>#tone</code> $\\to$ 1.0 s tone at 50% vol<br/>"
                "• <code>#tone=5</code> $\\to$ 0.5 s tone at 50% vol<br/>"
                "• <code>#tone=10,0</code> $\\to$ 1.0 s tone at 100% vol<br/>"
                "• <code>#tone=20,2</code> $\\to$ 2.0 s tone at 25% vol",
                table_cell_code
            )
        ],
        [
            Paragraph("<b><code>#pause</code></b><br/>Silence Generator", table_cell_bold),
            Paragraph("<code>#pause</code><br/><code>#pause=M</code><br/><i>(def: M=5)</i>", table_cell_code),
            Paragraph(
                "Generates pure digital silence (<code>SilenceSound</code>) at 16 kHz.<br/>"
                "• <b><code>M</code>:</b> Duration in deciseconds (samples = $1600 \\times M$).<br/>"
                "Inserts natural acoustic pacing between phrases.",
                table_cell_style
            ),
            Paragraph(
                "• <code>#pause</code> $\\to$ 0.5 s pause (5 decisecs)<br/>"
                "• <code>#pause=1</code> $\\to$ 0.1 s brief pause<br/>"
                "• <code>#pause=10</code> $\\to$ 1.0 s pause<br/>"
                "• <code>#pause=20</code> $\\to$ 2.0 s pause",
                table_cell_code
            )
        ],
        [
            Paragraph("<b><code>#count</code></b><br/>Numeric Counter", table_cell_bold),
            Paragraph("<code>#count</code><br/><i>(starts at 1)</i>", table_cell_code),
            Paragraph(
                "Queries vocabulary word for current number: <code>vocabGetWord(std::to_string(count))</code>. "
                "Automatically increments <code>count++</code> after queuing. Resets to 1 on each new message.",
                table_cell_style
            ),
            Paragraph(
                "1st encounter speaks <code>\"1\"</code>;<br/>"
                "2nd encounter speaks <code>\"2\"</code>;<br/>"
                "3rd encounter speaks <code>\"3\"</code>.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>#ord</code></b><br/>Ordinal Counter", table_cell_bold),
            Paragraph("<code>#ord</code><br/><i>(starts at 1)</i>", table_cell_code),
            Paragraph(
                "Retrieves ordinal word: <code>vocabGetWord(getOrdinalWord(count))</code>. "
                "Supports range 1..9 (<code>\"first\"</code> .. <code>\"ninth\"</code>; &gt;9 emits silence). "
                "Auto-increments <code>count++</code>. Shares counter state with <code>#count</code>.",
                table_cell_style
            ),
            Paragraph(
                "Defect detail iteration:<br/>"
                "1st $\\to$ <code>\"first\"</code><br/>"
                "2nd $\\to$ <code>\"second\"</code><br/>"
                "3rd $\\to$ <code>\"third\"</code>",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>.</code> and <code>-</code></b><br/>Punctuation", table_cell_bold),
            Paragraph("Single-char tokens", table_cell_code),
            Paragraph(
                "When numbers are split by <code>breakDigits=true</code>, punctuation appears as isolated tokens. "
                "<code>parseTask</code> translates them before vocab lookup:<br/>"
                "• <code>\".\"</code> $\\to$ <code>\"point\"</code> (looks up <code>vocabGetWord(\"point\")</code>)<br/>"
                "• <code>\"-\"</code> $\\to$ <code>\"minus\"</code> (looks up <code>vocabGetWord(\"minus\")</code>)",
                table_cell_style
            ),
            Paragraph(
                "• <code>\"3 4 6 . 9\"</code> $\\to$ \"three four six point nine\"<br/>"
                "• <code>\"- 5\"</code> $\\to$ \"minus five\"",
                table_cell_style
            )
        ],
        [
            Paragraph("<b>Vocabulary &amp; SFX Fallback</b>", table_cell_bold),
            Paragraph("Standard words", table_cell_code),
            Paragraph(
                "Standard words and digits (<code>\"0\"</code>–<code>\"9\"</code>) are loaded via <code>vocabGetWord(token)</code>. "
                "<b>Diagnostic Fallback:</b> If <code>vocabGetWord()</code> returns <code>NULL</code>, <code>parseTask</code> plays "
                "the sound effect <b><code>\"horn\"</code></b> (<code>PneumaticHornDouble3-16k.wav</code>). "
                "This gives an unmistakable auditory alert during testing if a word is missing from vocabulary clips.",
                table_cell_style
            ),
            Paragraph(
                "Missing word during bench test immediately triggers a loud dual train horn blast.",
                table_cell_style
            )
        ]
    ]

    t_s2 = Table(s2_data, colWidths=[85, 75, 195, 149])
    t_s2.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_s2)
    elements.append(Spacer(1, 4))

    # Section 4: Workflows
    elements.append(Paragraph("4. End-to-End Detector Message Workflows", h1_style))

    # Workflow 1: Real-time Defect Alert
    wf1_data = [
        [
            Paragraph("<b>Workflow 1: Real-Time Defect Alert (Hot Journal on Axle 48, Track 2 South Rail)</b>", table_header_style),
        ],
        [
            Paragraph(
                "• <b>Raw Spoken:</b> <code>\"#tone #track hot journal Axle #axle #rail Rail\"</code> | <b>Display:</b> <code>\"#track\\Hot Journal\\Axle #axle #rail Rail\"</code><br/>"
                "• <b>Stage 1 Spoken (breakDigits=true):</b> <code>\"#tone track 2 hot journal axle 4 8 south rail\"</code><br/>"
                "• <b>Stage 2 Audio Playback:</b> [1.0s 1kHz Tone @ 50% vol] $\\to$ <code>\"track\"</code> $\\to$ <code>\"2\"</code> $\\to$ <code>\"hot\"</code> $\\to$ <code>\"journal\"</code> $\\to$ <code>\"axle\"</code> $\\to$ <code>\"4\"</code> $\\to$ <code>\"8\"</code> $\\to$ <code>\"south\"</code> $\\to$ <code>\"rail\"</code><br/>"
                "• <b>Stage 1 LCD Display:</b> Line 0: <code>Track 2</code> | Line 1: <code>Hot Journal</code> | Line 2: <code>Axle 48 South Rail</code>",
                callout_style
            )
        ]
    ]
    t_wf1 = Table(wf1_data, colWidths=[504])
    t_wf1.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#2B6CB0")),
        ('BACKGROUND', (0,1), (-1,1), colors.HexColor("#F7FAFC")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#2B6CB0")),
        ('PADDING', (0,0), (-1,-1), 3),
    ]))
    elements.append(t_wf1)
    elements.append(Spacer(1, 3))

    # Workflow 2: Clean Train Exit Summary
    wf2_data = [
        [
            Paragraph("<b>Workflow 2: Clean Train Exit Summary (124 Axles, 45 mph, Milepost 346.9, 72°F)</b>", table_header_style),
        ],
        [
            Paragraph(
                "• <b>Raw Spoken:</b> <code>\"... milepost #milepost #track #direction no defects repeat no defects total axles #axles train speed #speed temperature #temp degrees detector out\"</code><br/>"
                "• <b>Raw Display:</b> <code>\"#track\\NO DEFECTS\\AXLES:#axles:3  SPEED:#speed:3\"</code><br/>"
                "• <b>Stage 1 Spoken:</b> <code>\"... milepost 3 4 6 . 9 track 2 southbound no defects repeat no defects total axles 1 2 4 train speed 4 5 temperature 7 2 degrees detector out\"</code><br/>"
                "• <b>Stage 2 Audio:</b> <code>.</code> spoken as <code>\"point\"</code>; all digits (<code>\"3 4 6 9 1 2 4 4 5 7 2\"</code>) spoken individually.<br/>"
                "• <b>Stage 1 LCD Display:</b> Line 0: <code>Track 2</code> | Line 1: <code>NO DEFECTS</code> | Line 2: <code>AXLES:124  SPEED: 45</code>",
                callout_style
            )
        ]
    ]
    t_wf2 = Table(wf2_data, colWidths=[504])
    t_wf2.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#2B6CB0")),
        ('BACKGROUND', (0,1), (-1,1), colors.HexColor("#F7FAFC")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#2B6CB0")),
        ('PADDING', (0,0), (-1,-1), 3),
    ]))
    elements.append(t_wf2)
    elements.append(Spacer(1, 3))

    # Workflow 3: Defect Exit Summary
    wf3_data = [
        [
            Paragraph("<b>Workflow 3: Train Exit with Defect List Summary (Multiple Defects)</b>", table_header_style),
        ],
        [
            Paragraph(
                "• <b>Detail Template:</b> <code>\"#ord hot journal Axle #axle #rail Rail\"</code> | <b>Exit Template:</b> <code>\"... you have a defect #defectlist total axles #axles detector out\"</code><br/>"
                "• <b>Pre-transformed Detail Messages in <code>data.defects</code>:</b><br/>"
                "&nbsp;&nbsp;&nbsp;&nbsp;Defect 1: <code>\"first hot journal axle 1 2 north rail\"</code> | Defect 2: <code>\"second dragging equipment near axle 8 4\"</code><br/>"
                "• <b>Expanded Exit Message:</b> <code>\"... you have a defect first hot journal axle 1 2 north rail second dragging equipment near axle 8 4 total axles 1 2 4 detector out\"</code>",
                callout_style
            )
        ]
    ]
    t_wf3 = Table(wf3_data, colWidths=[504])
    t_wf3.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#2B6CB0")),
        ('BACKGROUND', (0,1), (-1,1), colors.HexColor("#F7FAFC")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#2B6CB0")),
        ('PADDING', (0,0), (-1,-1), 3),
    ]))
    elements.append(t_wf3)

    # ==================== PAGE 3: TESTS & SUMMARY CHEAT SHEET ====================
    elements.append(PageBreak())

    elements.append(Paragraph("5. Unit Test Suite Matrix (<code>src/test/messages/test_messages.cpp</code>)", h1_style))
    elements.append(Paragraph(
        "Standalone verification using mock Arduino/FreeRTOS/NVS headers. All 27 tests pass with 100% coverage (<code>./run</code>).",
        body_style
    ))

    test_rows = [
        [
            Paragraph("#", table_header_style),
            Paragraph("Input Template", table_header_style),
            Paragraph("Expected Output", table_header_style),
            Paragraph("Mode", table_header_style),
            Paragraph("Tested Feature / Formatting Rule", table_header_style)
        ]
    ]

    test_cases_data = [
        (1, "#milepost", "3 4 6 . 9", "Spoken", "Milepost spoken mode: splits digits and decimal point"),
        (2, "#milepost", "346.9", "Display", "Milepost display mode: compact representation"),
        (3, "#milepost:3.1", "3 4 6 . 9", "Spoken", "Spoken mode ignores integer width constraints"),
        (4, "#milepost:4.1", " 346.9", "Display", "Right-aligned milepost, padded with 1 leading space"),
        (5, "#milepost:-4.1", "346.9 ", "Display", "Left-aligned milepost, padded with 1 trailing space"),
        (6, "#milepost:6.1", "   346.9", "Display", "Right-aligned milepost, padded with 3 leading spaces"),
        (7, "#milepost:-6.1", "346.9   ", "Display", "Left-aligned milepost, padded with 3 trailing spaces"),
        (8, "#axle", "4", "Display", "Single-digit axle number"),
        (9, "#axle:3", "  4", "Display", "Right-aligned axle number in 3-column field"),
        (10, "#axles:-3", "1 2 4", "Spoken", "Spoken total axles: negative width ignored, digits split"),
        (11, "#track", "Main 1", "Display", "Unmodified track name string"),
        (12, "#track:4", "Main", "Display", "Track string truncation (6 chars truncated to 4)"),
        (13, "#track:8", "  Main 1", "Display", "Right-aligned track name padded to 8 columns"),
        (14, "#track:-8", "Main 1  ", "Display", "Left-aligned track name padded to 8 columns"),
        (15, "#direction", "Northbound", "Display", "Direction 1 lookup (data.direction = 1)"),
        (16, "#direction:4", "Sout", "Display", "Direction 2 truncation (data.direction = 2 truncated to 4)"),
        (17, "#direction:12", "  Northbound", "Display", "Direction 1 right-aligned to 12 columns"),
        (18, "#direction:-12", "Southbound  ", "Display", "Direction 2 left-aligned to 12 columns"),
        (19, "#direction", "", "Display", "Undefined direction (direction = 0) returns empty string"),
        (20, "#direction", "", "Display", "Out-of-bounds direction (direction = 12) returns empty string"),
        (21, "#speed", "4 5", "Spoken", "Spoken speed: splits digits (45 -> \"4 5\")"),
        (22, "#temp", "72", "Display", "Display temperature from TemperatureManager"),
        (23, "#defectlist", "HOT_BOX DRAG_EQ", "Display", "Defect list concatenation with space separator"),
        (24, "milepost #milepost speed #speed:3 mph", "milepost 346.9 speed  45 mph", "Display", "Compound sentence with mixed tokens and modifiers"),
        (25, "Axles:#axles", "Axles:124", "Display", "Adjacent literal text and token"),
        (26, "Axles:#axles\\Speed:#speed", "Axles:124\\nSpeed:45", "Display", "Single backslash converted to LCD newline"),
        (27, "Axles:\\\\#axles", "Axles:\\124", "Display", "Double backslash escaped to literal backslash")
    ]

    for t_num, inp, exp, mode, feat in test_cases_data:
        test_rows.append([
            Paragraph(str(t_num), test_cell_bold),
            Paragraph(f"<code>{inp}</code>", test_cell_code),
            Paragraph(f"<code>{exp}</code>", test_cell_code),
            Paragraph(mode, test_cell_style),
            Paragraph(feat, test_cell_style)
        ])

    t_tests = Table(test_rows, colWidths=[16, 139, 125, 44, 180])
    t_tests.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 0.6),
        ('BOTTOMPADDING', (0,0), (-1,-1), 0.6),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_tests)
    elements.append(Spacer(1, 3))

    # Section 6: Quick Reference Summary Cheat Sheet
    elements.append(Paragraph("6. Quick Reference Summary Table", h1_style))

    cheat_data = [
        [
            Paragraph("Directive", table_header_style),
            Paragraph("Stage", table_header_style),
            Paragraph("Syntax", table_header_style),
            Paragraph("Default", table_header_style),
            Paragraph("Mode Sensitivity", table_header_style),
            Paragraph("Operational Purpose", table_header_style)
        ],
        [
            Paragraph("<code>\\</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>\\</code>", test_cell_code),
            Paragraph("—", test_cell_style),
            Paragraph("Both modes", test_cell_style),
            Paragraph("LCD newline delimiter for row separation", test_cell_style)
        ],
        [
            Paragraph("<code>\\\\</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>\\\\</code>", test_cell_code),
            Paragraph("—", test_cell_style),
            Paragraph("Both modes", test_cell_style),
            Paragraph("Escaped literal backslash character", test_cell_style)
        ],
        [
            Paragraph("<code>#milepost</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N[.M]</code>", test_cell_code),
            Paragraph("<code>1.1</code>", test_cell_style),
            Paragraph("Digits split if spoken; padded if display", test_cell_style),
            Paragraph("Milepost location (tenths of a mile)", test_cell_style)
        ],
        [
            Paragraph("<code>#track</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N</code>", test_cell_code),
            Paragraph("Verbatim", test_cell_style),
            Paragraph("Truncates to <code>|N|</code>; pads left/right", test_cell_style),
            Paragraph("Track identification name string", test_cell_style)
        ],
        [
            Paragraph("<code>#direction</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N</code>", test_cell_code),
            Paragraph("Verbatim", test_cell_style),
            Paragraph("Truncates/pads; empty if unknown", test_cell_style),
            Paragraph("Travel direction string (Northbound, etc.)", test_cell_style)
        ],
        [
            Paragraph("<code>#rail</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N</code>", test_cell_code),
            Paragraph("Verbatim", test_cell_style),
            Paragraph("Slash partition on odd/even rail index", test_cell_style),
            Paragraph("Defect rail side designation", test_cell_style)
        ],
        [
            Paragraph("<code>#axle</code>, <code>#axles</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N</code>", test_cell_code),
            Paragraph("<code>1</code>", test_cell_style),
            Paragraph("Digits split if spoken; padded if display", test_cell_style),
            Paragraph("Defect axle location / Total train axle count", test_cell_style)
        ],
        [
            Paragraph("<code>#speed</code>, <code>#temp</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("<code>:[±]N</code>", test_cell_code),
            Paragraph("<code>1</code>", test_cell_style),
            Paragraph("Digits split if spoken; padded if display", test_cell_style),
            Paragraph("Train speed (mph) / Ambient temperature", test_cell_style)
        ],
        [
            Paragraph("<code>#defectlist</code>", test_cell_code),
            Paragraph("Stage 1", test_cell_style),
            Paragraph("None", test_cell_style),
            Paragraph("—", test_cell_style),
            Paragraph("Space-separated list", test_cell_style),
            Paragraph("Accumulated defect detail summary strings", test_cell_style)
        ],
        [
            Paragraph("<code>#tone</code>", test_cell_code),
            Paragraph("Stage 2", test_cell_style),
            Paragraph("<code>[=M[,N]]</code>", test_cell_code),
            Paragraph("<code>10,1</code>", test_cell_style),
            Paragraph("Audio waveform synthesis", test_cell_style),
            Paragraph("1 kHz sine wave: $M$ decisecs, $1/2^N$ volume", test_cell_style)
        ],
        [
            Paragraph("<code>#pause</code>", test_cell_code),
            Paragraph("Stage 2", test_cell_style),
            Paragraph("<code>[=M]</code>", test_cell_code),
            Paragraph("<code>5</code>", test_cell_style),
            Paragraph("Audio waveform synthesis", test_cell_style),
            Paragraph("Digital silence: $M$ decisecs duration", test_cell_style)
        ],
        [
            Paragraph("<code>#count</code>, <code>#ord</code>", test_cell_code),
            Paragraph("Stage 2", test_cell_style),
            Paragraph("None", test_cell_style),
            Paragraph("<code>1</code>", test_cell_style),
            Paragraph("Word lookup &amp; auto-increment", test_cell_style),
            Paragraph("Numeric (<code>\"1\"</code>) / Ordinal (<code>\"first\"</code>..<code>\"ninth\"</code>) counter", test_cell_style)
        ],
        [
            Paragraph("<code>.</code> , <code>-</code>", test_cell_code),
            Paragraph("Stage 2", test_cell_style),
            Paragraph("Single char", test_cell_code),
            Paragraph("—", test_cell_style),
            Paragraph("Mapped to <code>\"point\"</code> / <code>\"minus\"</code>", test_cell_style),
            Paragraph("Pronounces decimal point and negative signs", test_cell_style)
        ],
        [
            Paragraph("<code>&lt;word&gt;</code>", test_cell_code),
            Paragraph("Stage 2", test_cell_style),
            Paragraph("String", test_cell_code),
            Paragraph("—", test_cell_style),
            Paragraph("ROM/SD WAV clip lookup", test_cell_style),
            Paragraph("Speaks vocabulary clip; falls back to horn SFX", test_cell_style)
        ]
    ]

    t_cheat = Table(cheat_data, colWidths=[65, 45, 60, 40, 124, 170])
    t_cheat.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 0.6),
        ('BOTTOMPADDING', (0,0), (-1,-1), 0.6),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_cheat)

    doc.build(elements, canvasmaker=NumberedCanvas)
    print(f"[OK] Generated PDF: {filename}")

if __name__ == "__main__":
    out_pdf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PDF
    build_pdf(out_pdf)
