#include "src/klfbackend.h"
#include <qfile.h>
#include <QtCore>

int main(int argc, char ** argv)
{
    QCoreApplication example(argc, argv);

    KLFBackend::klfSettings settings;
    bool ok = KLFBackend::detectSettings(&settings);
    if (!ok) {
        throw std::runtime_error("error in your system: are latex,dvips and gs installed?");
        return EXIT_FAILURE;
    }

    KLFBackend::klfInput input;
    input.latex = "\\int_{\\Sigma}\\!(\\vec{\\nabla}\\times\\vec u)\\,d\\vec S ="
                  " \\oint_C \\vec{u}\\cdot d\\vec r";
    input.mathmode = "\\[ ... \\]";
//    input.preamble = "\\usepackage{somerequiredpackage}\n";
    input.fg_color = qRgb(255, 168, 88); // beige
    input.bg_color = qRgba(0, 64, 64, 255); // dark turquoise
    input.dpi = 300;

    KLFBackend::klfOutput out = KLFBackend::getLatexFormula(input, settings);

    if (out.status != 0) {
        // an error occurred. an appropriate error string is in out.errorstr
        fprintf(stderr, "Problem in getLatexFormula: %s", out.errorstr.toStdString().c_str());
        return EXIT_FAILURE;
    }

    // write contents of 'out.pdfdata' to a file to get a PDF file (for example)
    {
        QFile fpdf("teste.pdf");

        fpdf.open(QFile::WriteOnly);
        fpdf.write(out.pdfdata);
    }

    return example.exec();
}
