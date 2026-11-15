#!/usr/bin/env python3
"""
Generate a publication-grade PDF specification and implementation plan for the
SD Card External Configuration Parser (config.txt) for the ckt-defect firmware.
"""

import os
import sys
import shutil
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable, Preformatted
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
            self.drawString(54, 752, "Defect Detector | SD Card External Configuration Architecture & Plan")
            self.setStrokeColor(colors.HexColor("#CBD5E0"))
            self.setLineWidth(0.5)
            self.line(54, 746, 558, 746)

        # Footer (all pages)
        self.setStrokeColor(colors.HexColor("#CBD5E0"))
        self.setLineWidth(0.5)
        self.line(54, 45, 558, 45)
        
        self.drawString(54, 32, "Confidential - Engineering Architectural Specification & Implementation Plan")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(558, 32, page_str)
        self.restoreState()

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_PDF = os.path.join(SCRIPT_DIR, "external_configuration_plan.pdf")

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
        fontSize=16.5,
        leading=20.5,
        textColor=colors.HexColor("#1A365D"),
        spaceAfter=3
    )

    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9.5,
        leading=12.5,
        textColor=colors.HexColor("#2B6CB0"),
        spaceAfter=5
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
        fontSize=10.5,
        leading=13.5,
        textColor=colors.HexColor("#1A365D"),
        spaceBefore=6,
        spaceAfter=2.5,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.8,
        leading=10.8,
        textColor=colors.HexColor("#2D3748"),
        spaceAfter=3
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=6.8,
        leading=8.8,
        textColor=colors.white,
        alignment=1 # Center
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=6.5,
        leading=8.2,
        textColor=colors.HexColor("#2D3748")
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=6.5,
        leading=8.2,
        textColor=colors.HexColor("#1A365D")
    )

    table_cell_code = ParagraphStyle(
        'TableCellCode',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=6.2,
        leading=7.8,
        textColor=colors.HexColor("#1A202C")
    )

    callout_style = ParagraphStyle(
        'CalloutText',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.4,
        leading=9.8,
        textColor=colors.HexColor("#1A365D")
    )

    code_block_style = ParagraphStyle(
        'CodeBlock',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=5.4,
        leading=6.6,
        textColor=colors.HexColor("#1A202C")
    )

    screen_mockup_style = ParagraphStyle(
        'ScreenMockup',
        parent=styles['Normal'],
        fontName='Courier-Bold',
        fontSize=6.5,
        leading=8.0,
        textColor=colors.HexColor("#1A202C")
    )

    elements = []

    # ==================== PAGE 1: OBJECTIVES, GRAMMAR & KEY TABLES ====================
    elements.append(Paragraph("SD Card External Configuration Parser Architecture", title_style))
    elements.append(Paragraph("Technical Plan of Attack, Syntax Grammar, Validation Rules, and Design Trade-offs", subtitle_style))

    meta_text = "<b>Firmware:</b> ESP32-S2 (<code>ckt-defect</code>) | <b>Source Target:</b> <code>0:config.txt</code> (FatFs) | <b>Status:</b> Engineering Plan & Proposal"
    elements.append(Paragraph(meta_text, meta_style))
    elements.append(Spacer(1, 2))
    elements.append(HRFlowable(width="100%", thickness=1.2, color=colors.HexColor("#1A365D"), spaceAfter=4))

    elements.append(Paragraph("1. Executive Summary & Operational Requirements", h1_style))
    elements.append(Paragraph(
        "The objective is to implement a robust, fail-safe configuration file parser that reads runtime customizations from an optional "
        "<code>config.txt</code> file in the root directory of the MicroSD card. This allows operators to customize messages, track/direction names, "
        "and defect rates without modifying firmware or using the physical LCD menu.",
        body_style
    ))

    # Core Requirements Callout
    req_box_data = [
        [
            Paragraph(
                "• <b>Presence Flag:</b> If <code>config.txt</code> is present, <code>cfg.externalConfigPresent = true</code>.<br/>"
                "• <b>Fail-Fast &amp; Freeze on Error:</b> Stop parsing at the <i>first error</i> (syntax, type mismatch, range violation, illegal character). "
                "Display error diagnosis and line number on the 20x4 LCD; freeze there with a single <b><code>REBOOT</code></b> button (Button 4 / col 16) "
                "that triggers an ESP32 hardware reset.<br/>"
                "• <b>Success Banner:</b> If parsing succeeds completely, show a brief (2s) <code>\"External Config Loaded\"</code> LCD message.<br/>"
                "• <b>Case Policy:</b> Keys are strictly case-insensitive (e.g. <code>entrancemsg == entranceMsg</code>); string values preserve case verbatim.<br/>"
                "• <b>Selective Overrides &amp; Menu Hiding:</b> Overridden variables hide their corresponding LCD menus and disable ID-to-string updates. "
                "If any message is defined in <code>config.txt</code>, default message setup is skipped and all unprovided messages remain empty (<code>\"\"</code>).<br/>"
                "• <b>NVM Protection:</b> Variables sourced from <code>config.txt</code> (e.g. <code>vocabSelected</code>) must <b>never</b> be written to NVM.",
                callout_style
            )
        ]
    ]
    t_req = Table(req_box_data, colWidths=[504])
    t_req.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EBF8FF")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#3182CE")),
        ('PADDING', (0,0), (-1,-1), 3.5),
    ]))
    elements.append(t_req)
    elements.append(Spacer(1, 4))

    # Section 2: File Syntax & Key Tables
    elements.append(Paragraph("2. Configuration File Format & Valid Key Catalog", h1_style))
    elements.append(Paragraph(
        "The configuration file consists of <code>key = value</code> pairs in arbitrary order. Comments start with <code>#</code> or <code>;</code>. "
        "Leading and trailing whitespace around keys and values are trimmed.",
        body_style
    ))

    # Table of Global Keys
    global_keys_data = [
        [
            Paragraph("Config Key", table_header_style),
            Paragraph("Type", table_header_style),
            Paragraph("Destination Field", table_header_style),
            Paragraph("Validation &amp; Allowed Range", table_header_style),
            Paragraph("Hidden Menu Node", table_header_style)
        ],
        [
            Paragraph("<code>trackName1</code><br/><i>(alias: trackNameA)</i>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.trackName[0]</code>", table_cell_code),
            Paragraph("Allowed string chars; max 16 chars; preserves case", table_cell_style),
            Paragraph("Track A Name", table_cell_style)
        ],
        [
            Paragraph("<code>trackName2</code><br/><i>(alias: trackNameB)</i>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.trackName[1]</code>", table_cell_code),
            Paragraph("Allowed string chars; max 16 chars; preserves case", table_cell_style),
            Paragraph("Track B Name", table_cell_style)
        ],
        [
            Paragraph("<code>direction1Name</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.direction1Name</code>", table_cell_code),
            Paragraph("Allowed string chars; max 16 chars; preserves case", table_cell_style),
            Paragraph("Direction 1 Name", table_cell_style)
        ],
        [
            Paragraph("<code>direction2Name</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.direction2Name</code>", table_cell_code),
            Paragraph("Allowed string chars; max 16 chars; preserves case", table_cell_style),
            Paragraph("Direction 2 Name", table_cell_style)
        ],
        [
            Paragraph("<code>railName</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.railName</code>", table_cell_code),
            Paragraph("Slash-separated sides: e.g. <code>\"North/South\"</code>", table_cell_style),
            Paragraph("Rail Names", table_cell_style)
        ],
        [
            Paragraph("<code>vocabSelected</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>cfg.vocabSelected</code>", table_cell_code),
            Paragraph("Alphanumeric string; SD existence validated later during vocab load", table_cell_style),
            Paragraph("Vocab Select", table_cell_style)
        ],
        [
            Paragraph("<code>integrityTrainRate</code>", table_cell_code),
            Paragraph("Integer", table_cell_style),
            Paragraph("<code>trackMessages.integrityTrainRate</code>", table_cell_code),
            Paragraph("Integer in range 1 .. 999,999 trains (matching 6-digit menu limits)", table_cell_style),
            Paragraph("Random Integrity Rate", table_cell_style)
        ]
    ]

    t_glob = Table(global_keys_data, colWidths=[90, 42, 125, 147, 100])
    t_glob.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 1.2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.2),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_glob)
    elements.append(Spacer(1, 3))

    # Table of Message Keys
    msg_keys_data = [
        [
            Paragraph("Message Config Key", table_header_style),
            Paragraph("Type", table_header_style),
            Paragraph("Destination in <code>MessageBundle</code>", table_header_style),
            Paragraph("Operational Context &amp; Purpose", table_header_style)
        ],
        [
            Paragraph("<code>entranceMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.entranceMsg</code>", table_cell_code),
            Paragraph("Broadcast when train first arrives at detector", table_cell_style)
        ],
        [
            Paragraph("<code>exitCleanMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.exitCleanMsg</code>", table_cell_code),
            Paragraph("Spoken summary played after clean train departure", table_cell_style)
        ],
        [
            Paragraph("<code>exitCleanDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.exitCleanDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown after clean train departure (lines separated by <code>\\</code>)", table_cell_style)
        ],
        [
            Paragraph("<code>exitDefectMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.exitDefectMsg</code>", table_cell_code),
            Paragraph("Spoken exit summary played when defects detected (contains <code>#defectlist</code>)", table_cell_style)
        ],
        [
            Paragraph("<code>exitDefectDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.exitDefectDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown when defect detected (lines separated by <code>\\</code>)", table_cell_style)
        ],
        [
            Paragraph("<code>integrityMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.integrityMsg</code>", table_cell_code),
            Paragraph("Spoken message played on simulated integrity failure", table_cell_style)
        ],
        [
            Paragraph("<code>integrityDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.integrityDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown on simulated integrity failure", table_cell_style)
        ],
        [
            Paragraph("<code>minAxleMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.minAxleMsg</code>", table_cell_code),
            Paragraph("Spoken phrase if train departs with fewer than minimum required axles", table_cell_style)
        ],
        [
            Paragraph("<code>minAxleDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.minAxleDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown when train departs with too few axles", table_cell_style)
        ],
        [
            Paragraph("<code>tooSlowMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.tooSlowMsg</code>", table_cell_code),
            Paragraph("Spoken phrase played if train speed is below minimum programmed threshold", table_cell_style)
        ],
        [
            Paragraph("<code>tooSlowDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.tooSlowDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown when train speed is below threshold", table_cell_style)
        ],
        [
            Paragraph("<code>detectorBlockedMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.detectorBlockedMsg</code>", table_cell_code),
            Paragraph("Spoken phrase played if an axle stops in front of sensor longer than timeout", table_cell_style)
        ],
        [
            Paragraph("<code>detectorBlockedDisplayMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.detectorBlockedDisplayMsg</code>", table_cell_code),
            Paragraph("20x4 LCD text shown when sensor remains blocked", table_cell_style)
        ],
        [
            Paragraph("<code>excessAlarmsMsg</code>", table_cell_code),
            Paragraph("String", table_cell_style),
            Paragraph("<code>trackMessages.excessAlarmsMsg</code>", table_cell_code),
            Paragraph("Phrase appended to defect list if defect count exceeds <code>maxDefects</code>", table_cell_style)
        ]
    ]

    t_msg = Table(msg_keys_data, colWidths=[110, 36, 150, 208])
    t_msg.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 1.0),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.0),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_msg)

    # ==================== PAGE 2: DEFECT GROUPING, CODE PLACEMENT & FILE CHANGES ====================
    elements.append(PageBreak())

    elements.append(Paragraph("3. Defect Message Grouping (`[defect]`) & File Example", h1_style))
    elements.append(Paragraph(
        "Defect messages (<code>alertMsg</code>, <code>displayMsg</code>, <code>detailMsg</code>, <code>axleRate</code>) can be provided "
        "multiple times to define arbitrary custom defect types. Each group adds an entry to <code>MessageBundle.defects</code>. "
        "Standard INI section headers <code>[defect]</code> delimit groups cleanly. Each block strictly requires all 4 keys; incomplete blocks trigger a syntax error:",
        body_style
    ))

    # Example snippet box: 2-column layout preserving exact lines
    example_col1 = (
        "# General Hardware & Identity Settings\n"
        "trackName1         = Main 1\n"
        "trackName2         = Siding\n"
        "direction1Name     = Eastbound\n"
        "direction2Name     = Westbound\n"
        "railName           = North/South\n"
        "vocabSelected      = default\n"
        "integrityTrainRate = 200\n"
        "\n"
        "# Broadcast Message Templates\n"
        "entranceMsg         = Defect Detector MP #milepost #track #direction\n"
        "exitCleanMsg        = Defect Detector MP #milepost #track no defects #axles\n"
        "exitCleanDisplayMsg = #track\\NO DEFECTS\\AXLES:#axles:3  SPEED:#speed:3\n"
        "exitDefectMsg       = Defect Detector MP #milepost defect #defectlist\n"
        "exitDefectDisplayMsg= #track\\DEFECT DETECTED!\\AXLES:#axles:3  SPEED:#speed:3\n"
        "integrityMsg        = Defect Detector integrity failure\n"
        "tooSlowMsg          = Defect Detector train 2 slow\n"
        "detectorBlockedMsg  = Defect Detector detector blocked\n"
        "excessAlarmsMsg     = excessive alarms"
    )

    example_col2 = (
        "# Defect 1: Hot Box / Journal\n"
        "[defect]\n"
        "alertMsg   = #tone #track hot journal Axle #axle #rail Rail\n"
        "displayMsg = #track\\Hot Journal\\Axle #axle #rail Rail\n"
        "detailMsg  = #ord hot journal Axle #axle #rail Rail\n"
        "axleRate   = 150\n"
        "\n"
        "# Defect 2: Dragging Equipment Detector\n"
        "[defect]\n"
        "alertMsg   = #tone #track dragging equipment near Axle #axle\n"
        "displayMsg = #track\\Dragging Equip\\Near Axle #axle\n"
        "detailMsg  = #ord dragging equipment near Axle #axle\n"
        "axleRate   = 250\n"
        "\n"
        "# Defect 3: High Load\n"
        "[defect]\n"
        "alertMsg   = #tone #track high load near Axle #axle\n"
        "displayMsg = #track\\High Load\\Near Axle #axle\n"
        "detailMsg  = #ord high load near Axle #axle\n"
        "axleRate   = 500"
    )

    snippet_cells = [
        [
            Preformatted(example_col1, code_block_style),
            Preformatted(example_col2, code_block_style)
        ]
    ]

    t_snippet = Table(snippet_cells, colWidths=[256, 248])
    t_snippet.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#F7FAFC")),
        ('BOX', (0,0), (-1,-1), 0.8, colors.HexColor("#CBD5E0")),
        ('LINEBEFORE', (1,0), (1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('LEFTPADDING', (0,0), (-1,-1), 4),
        ('RIGHTPADDING', (0,0), (-1,-1), 4),
    ]))
    elements.append(t_snippet)
    elements.append(Spacer(1, 4))

    # Section 4: Architecture & Code Placement Proposal
    elements.append(Paragraph("4. Architecture & Code Placement Proposal", h1_style))
    elements.append(Paragraph(
        "To ensure modularity and facilitate host unit testing, we propose placing the parser in a dedicated module rather than cluttering <code>ckt-defect.ino</code>:",
        body_style
    ))

    arch_rows = [
        [
            Paragraph("Module / Source File", table_header_style),
            Paragraph("Nature of Change", table_header_style),
            Paragraph("Specific Responsibilities &amp; Implementation Details", table_header_style)
        ],
        [
            Paragraph("<b><code>config-file.h</code></b><br/><b><code>config-file.cpp</code></b><br/><i>(New Files)</i>", table_cell_bold),
            Paragraph("New Core Module", table_cell_style),
            Paragraph(
                "• Implements <code>loadExternalConfigFile(filepath, cfg, msgs, lcd)</code> using FatFs.<br/>"
                "• Line tokenizer, whitespace trimmer, case-insensitive comparison, trackNameA/B aliases.<br/>"
                "• Strict validators: integer bounds (axleRate 1..999,999; integrity 1..999,999), allowed characters.<br/>"
                "• Defers `#` tokens to runtime (<code>transformMessage</code>) and vocab SD check to vocab loader.<br/>"
                "• Strict 4-key <code>[defect]</code> section state machine assembling <code>DefectMessage</code> objects.<br/>"
                "• LCD error screen and freezing reboot loop on failure; 2.0s success banner on load.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>configuration.h</code></b>", table_cell_bold),
            Paragraph("Data Structure Extension", table_cell_style),
            Paragraph(
                "• Add volatile tracking booleans: <code>externalConfigPresent</code> (existing), <code>externalMessagesPresent</code>, "
                "<code>externalTrackNamePresent[2]</code>, <code>externalDirection1NamePresent</code>, <code>externalDirection2NamePresent</code>, "
                "<code>externalRailNamePresent</code>, <code>externalVocabSelectedPresent</code>, <code>externalIntegrityTrainRatePresent</code>.<br/>"
                "• Marked <code>// Volatile value, not stored in NVM</code> so <code>tools/gen_codec.py</code> ignores them.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>configuration.cpp</code></b>", table_cell_bold),
            Paragraph("Logic Guarding", table_cell_style),
            Paragraph(
                "• <b><code>saveConfiguration</code>:</b> Guard <code>vocabSelected</code> write so it is NEVER written to NVM if <code>externalVocabSelectedPresent == true</code>.<br/>"
                "• <b><code>updateTrackNames</code>:</b> Only overwrite <code>cfg-&gt;trackName[i]</code> from ID if <code>!cfg-&gt;externalTrackNamePresent[i]</code>.<br/>"
                "• <b><code>updateDirectionNames</code> / <code>updateRailNames</code>:</b> Guard with corresponding <code>external*Present</code> flags.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>messages.cpp</code></b>", table_cell_bold),
            Paragraph("Message Default Guarding", table_cell_style),
            Paragraph(
                "• In <code>updateMessages(trackMessages, cfg)</code>: if <code>cfg.externalMessagesPresent == true</code>, "
                "skip <code>setDefaultMessages()</code> completely so loaded templates are preserved and unprovided messages remain empty.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>menu-factory.cpp</code></b>", table_cell_bold),
            Paragraph("Dynamic Menu Hiding", table_cell_style),
            Paragraph(
                "• In <code>updateAllMenuVisibility(cfg)</code>:<br/>"
                "&nbsp;&nbsp;• If <code>externalMessagesPresent</code>: hide <code>Defects</code>, <code>Entrance Message</code>, <code>Live Defect Msgs</code>, "
                "<code>Talk Defect Only</code>, and all <code>Include*</code> submenus.<br/>"
                "&nbsp;&nbsp;• If <code>externalTrackNamePresent[0/1]</code>: hide Track A / Track B Name menus.<br/>"
                "&nbsp;&nbsp;• If <code>externalDirection1/2NamePresent</code>: hide Direction 1 / 2 Name menus.<br/>"
                "&nbsp;&nbsp;• If <code>externalRailNamePresent</code>: hide Rail Names menu.<br/>"
                "&nbsp;&nbsp;• If <code>externalVocabSelectedPresent</code>: hide Vocab Select menu.",
                table_cell_style
            )
        ],
        [
            Paragraph("<b><code>ckt-defect.ino</code></b>", table_cell_bold),
            Paragraph("Startup Reordering", table_cell_style),
            Paragraph(
                "• Move SD card mount earlier in <code>setup()</code> (immediately after splash display).<br/>"
                "• Execute <code>loadExternalConfigFile()</code> before <code>createAppMenu()</code> so menu trees build with correct visibility.<br/>"
                "• Delete old commented-out <code>config.txt</code> reader and legacy <code>configKeyValueSplit()</code>.",
                table_cell_style
            )
        ]
    ]

    t_arch = Table(arch_rows, colWidths=[100, 75, 329])
    t_arch.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_arch)

    # ==================== PAGE 3: STARTUP FLOW, ERROR UI, OPEN QUESTIONS & ROADMAP ====================
    elements.append(PageBreak())

    elements.append(Paragraph("5. Startup Lifecycle, Error Handling & Reboot UI", h1_style))
    elements.append(Paragraph(
        "Because configuration parsing occurs prior to menu activation, error handling renders directly to the LCD driver "
        "and holds the hardware in a fail-safe state.",
        body_style
    ))

    # Error UI Layout with strict monospaced Preformatted text
    screen_mockup_text = (
        "+--------------------+\n"
        "|Config Error: Line 7|\n"
        "|Unknown Key:        |\n"
        "|volume_gain         |\n"
        "|              REBOOT|\n"
        "+--------------------+\n"
        " [Btn1] [Btn2] [Btn3] [Btn4]"
    )

    err_layout_data = [
        [
            Paragraph("<b>LCD Error Screen Layout (20x4 Matrix)</b>", table_header_style),
            Paragraph("<b>Operational Flow &amp; Watchdog Management</b>", table_header_style)
        ],
        [
            Preformatted(screen_mockup_text, screen_mockup_style),
            Paragraph(
                "• <b>Fail-Fast Halt:</b> Parser stops immediately at the first offending line.<br/>"
                "• <b>Diagnostic Readout:</b> Row 0 prints line number; Rows 1–2 display specific error cause and key/token.<br/>"
                "• <b>Reboot Button (Button 4):</b> Display reads <code>REBOOT</code> right-aligned at col 14–19 above Button 4.<br/>"
                "• <b>Watchdog Safety:</b> Loops calling <code>esp_task_wdt_reset()</code> and <code>lcd-&gt;readKeys()</code>. "
                "Pressing Button 4 calls <code>restartWithDelay(500, lcd)</code> to reboot the microcontroller cleanly.",
                table_cell_style
            )
        ]
    ]

    t_err = Table(err_layout_data, colWidths=[180, 324])
    t_err.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('PADDING', (0,0), (-1,-1), 3),
        ('BACKGROUND', (0,1), (0,1), colors.HexColor("#F7FAFC")),
    ]))
    elements.append(t_err)
    elements.append(Spacer(1, 4))

    # Section 6: Resolved Architectural Decisions
    elements.append(Paragraph("6. Resolved Architectural Design Decisions", h1_style))
    elements.append(Paragraph(
        "Rigorous review with engineering leads has finalized the following seven architectural decisions:",
        body_style
    ))

    questions_data = [
        [
            Paragraph("#", table_header_style),
            Paragraph("Topic / Area", table_header_style),
            Paragraph("Engineering Decision", table_header_style),
            Paragraph("Implementation Details &amp; Operational Rationale", table_header_style)
        ],
        [
            Paragraph("1", table_cell_bold),
            Paragraph("<b>Defect Grouping Syntax</b>", table_cell_bold),
            Paragraph("<b>Standard <code>[defect]</code> Headers</b>", table_cell_style),
            Paragraph("Use standard case-insensitive INI <code>[defect]</code> section headers to allocate new <code>DefectMessage</code> instances.", table_cell_style)
        ],
        [
            Paragraph("2", table_cell_bold),
            Paragraph("<b>Incomplete Defect Blocks</b>", table_cell_bold),
            Paragraph("<b>Strict Fail-Fast Validation</b>", table_cell_style),
            Paragraph("All 4 keys (<code>alertMsg</code>, <code>displayMsg</code>, <code>detailMsg</code>, <code>axleRate</code>) required per defect; missing keys halt parser at line number.", table_cell_style)
        ],
        [
            Paragraph("3", table_cell_bold),
            Paragraph("<b>All-or-Nothing Defaults</b>", table_cell_bold),
            Paragraph("<b>Confirmed Intentional</b>", table_cell_style),
            Paragraph("Defining &ge;1 message skips all defaults. Unset messages remain empty (<code>\"\"</code>) and silent to support intentional audio silencing.", table_cell_style)
        ],
        [
            Paragraph("4", table_cell_bold),
            Paragraph("<b>String &amp; Token Validation</b>", table_cell_bold),
            Paragraph("<b>Character Filter Only</b>", table_cell_style),
            Paragraph("Allow <code>[a-zA-Z0-9 _\\-:\\.\\/\\\\=,#\\?!]</code>. Do NOT validate <code>#tokens</code> in config; runtime engine handles tokens &amp; plays horn.", table_cell_style)
        ],
        [
            Paragraph("5", table_cell_bold),
            Paragraph("<b><code>vocabSelected</code> Check</b>", table_cell_bold),
            Paragraph("<b>Deferred to Vocab Loader</b>", table_cell_style),
            Paragraph("Do NOT check SD directory existence in config parser. Firmware vocab loader validates existence later during audio initialization.", table_cell_style)
        ],
        [
            Paragraph("6", table_cell_bold),
            Paragraph("<b>Track Name Aliases</b>", table_cell_bold),
            Paragraph("<b>Canonical &amp; Aliases Supported</b>", table_cell_style),
            Paragraph("Support <code>trackName1/2</code> (canonical) and accept <code>trackNameA/B</code> (aliases) as case-insensitive keys.", table_cell_style)
        ],
        [
            Paragraph("7", table_cell_bold),
            Paragraph("<b>Integer Bounds Alignment</b>", table_cell_bold),
            Paragraph("<b>Range-Checked vs. Menus</b>", table_cell_style),
            Paragraph("• <code>axleRate</code>: 1 .. 999,999 axles.<br/>• <code>integrityTrainRate</code>: 1 .. 999,999 trains (matches menu limits). Halts on out-of-range.", table_cell_style)
        ]
    ]

    t_quest = Table(questions_data, colWidths=[14, 90, 160, 240])
    t_quest.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 1.5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.5),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_quest)
    elements.append(Spacer(1, 4))

    # Section 7: Phased Implementation Plan
    elements.append(Paragraph("7. Phased Implementation Plan", h1_style))

    plan_rows = [
        [
            Paragraph("Phase", table_header_style),
            Paragraph("Major Milestone Tasks &amp; Engineering Deliverables", table_header_style),
            Paragraph("Verification Gate", table_header_style)
        ],
        [
            Paragraph("<b>Phase 1</b>", table_cell_bold),
            Paragraph("<b>Data Structures &amp; Guards:</b> Add tracking booleans in <code>configuration.h</code>; update <code>configuration.cpp</code> (NVM write guard &amp; selective <code>update*Names</code>); update <code>messages.cpp</code> (skip defaults).", table_cell_style),
            Paragraph("Run <code>tools/gen_codec.py</code>; verify 100% NVM schema preservation.", table_cell_style)
        ],
        [
            Paragraph("<b>Phase 2</b>", table_cell_bold),
            Paragraph("<b>Standalone Parser Module:</b> Create <code>config-file.h/cpp</code> with tokenizer, validators (allowed chars, integer bounds), strict 4-key <code>[defect]</code> grouping, and track aliases. Unit test harness in <code>src/test/config/</code>.", table_cell_style),
            Paragraph("100% pass on unit test harness (syntax errors, range checks, group parsing).", table_cell_style)
        ],
        [
            Paragraph("<b>Phase 3</b>", table_cell_bold),
            Paragraph("<b>Menu Hiding Integration:</b> Update <code>updateAllMenuVisibility()</code> in <code>menu-factory.cpp</code> to selectively hide menus for messages, defects, tracks, directions, rail, and vocab.", table_cell_style),
            Paragraph("Inspect menu tree hierarchy under simulated config overrides.", table_cell_style)
        ],
        [
            Paragraph("<b>Phase 4</b>", table_cell_bold),
            Paragraph("<b>System Integration:</b> Move SD mount earlier in <code>ckt-defect.ino</code>; integrate <code>loadExternalConfigFile()</code>; implement LCD error screen, <code>REBOOT</code> button, and 2s success banner.", table_cell_style),
            Paragraph("Full firmware compilation via <code>make compile</code> (0 warnings/errors).", table_cell_style)
        ],
        [
            Paragraph("<b>Phase 5</b>", table_cell_bold),
            Paragraph("<b>Bench &amp; Hardware Verification:</b> Test complete SD card workflow with realistic <code>config.txt</code> files, deliberate syntax errors, and NVM isolation checks.", table_cell_style),
            Paragraph("End-to-end verified runtime behavior on target hardware.", table_cell_style)
        ]
    ]

    t_plan = Table(plan_rows, colWidths=[40, 314, 150])
    t_plan.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#1A365D")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 1.5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.5),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor("#F7FAFC")]),
    ]))
    elements.append(t_plan)

    doc.build(elements, canvasmaker=NumberedCanvas)
    print(f"[OK] Generated PDF: {filename}")

if __name__ == "__main__":
    out_pdf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PDF
    build_pdf(out_pdf)
