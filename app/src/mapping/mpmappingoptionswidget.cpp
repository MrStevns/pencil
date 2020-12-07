#include "mpmappingoptionswidget.h"

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QToolButton>
#include <QSplitter>
#include <QAction>
#include <QDebug>
#include <QComboBox>
#include <QScrollArea>
#include <QStandardItemModel>

#include <spinslider.h>
#include "brushsetting.h"
#include "editor.h"

#include "combobox.h"
#include "mpmappingwidget.h"

MPInputButton::MPInputButton(BrushInputType inputType, QWidget* parent)
    : QToolButton(parent), mInputType(inputType)
{
    connect(this, &QToolButton::pressed, this, &MPInputButton::pressed);
}

MPInputButton::MPInputButton(MPInputButton* inputButton)
    : QToolButton(inputButton)
{
    mInputType = inputButton->mInputType;
    connect(this, &QToolButton::pressed, this, &MPInputButton::pressed);
}

void MPInputButton::pressed()
{
    emit didPress(mInputType);
}

MPMappingOptionsWidget::MPMappingOptionsWidget(QString optionName, BrushSettingType settingType, QWidget* parent)
    : QDialog(parent, Qt::Tool), mSettingType(settingType)
{
    this->setWindowTitle(QString(optionName) + " " + tr("input mapping"));
}

void MPMappingOptionsWidget::initUI()
{
    setupUI();
}

void MPMappingOptionsWidget::setupUI()
{
    mHBoxLayout = new QHBoxLayout();
    setBaseSize(300,400);

    mHBoxLayout->setContentsMargins(5, 5, 5, 5);
    mGridLayout = new QGridLayout();
    mGridLayout->setContentsMargins(0, 0, 0, 0);
    QScrollArea* optionsScrollArea = new QScrollArea(this);
    optionsScrollArea->setWidgetResizable(true);
    optionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    optionsScrollArea->setMinimumWidth(300);
    optionsScrollArea->setMaximumHeight(500);

    QWidget* container = new QWidget(this);
    container->setLayout(mGridLayout);
    mGridLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    mMappingOptionsComboBox = new ComboBox(this);
    QLabel* descriptionLabel = new QLabel("Select an input from the dropdown\nto add a new input to map", this);
    descriptionLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    descriptionLabel->setContentsMargins(5,5,0,5);
    descriptionLabel->setMaximumSize(descriptionLabel->minimumSizeHint());

    mGridLayout->addWidget(descriptionLabel, 0, 0, 1, 0);
    mGridLayout->addWidget(mMappingOptionsComboBox, 1, 0, 1, 3);

    optionsScrollArea->setWidget(container);
    mHBoxLayout->addWidget(optionsScrollArea);

    QScrollArea* mappingScrollArea = new QScrollArea(this);
    mappingScrollArea->setWidgetResizable(true);
    mappingScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    mappingScrollArea->setMinimumWidth(300);
    mappingScrollArea->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    mappingScrollArea->setMaximumHeight(500);

    mVBoxLayout = new QVBoxLayout();
    mVBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    QWidget* mappingContainer = new QWidget(this);

    mappingContainer->setLayout(mVBoxLayout);
    mappingScrollArea->setWidget(mappingContainer);
    mHBoxLayout->addWidget(mappingScrollArea);

    for (int i = 0; i < static_cast<int>(BrushInputType::BRUSH_INPUTS_COUNT); i++) {

        BrushInputType input = static_cast<BrushInputType>(i);

        auto inputMapping = mEditor->getBrushInputMapping(mSettingType, input);

        mMappingOptionsComboBox->addItem(getBrushInputName(input), i);
        if (inputMapping.controlPoints.numberOfPoints > 0) {

            MPMappingOption option = createMappingOption(input);
            mMappingOptionsComboBox->setItemEnabled(i, false);

            mOptions << option;
        }
    }

    connect(mMappingOptionsComboBox, &ComboBox::activated, this, &MPMappingOptionsWidget::addOptionField);
    connect(this, &MPMappingOptionsWidget::notifyMappingWidgetNeedsUpdate, this, &MPMappingOptionsWidget::updateMappingWidgetForInput);

    setLayout(mHBoxLayout);
}

MPMappingOptionsWidget::MPMappingOption MPMappingOptionsWidget::createMappingOption(BrushInputType input)
{
    QLabel* settingDescLabel = new QLabel(getBrushInputName(input), nullptr);
    settingDescLabel->setContentsMargins(5,0,5,0);

    auto inputInfo = mEditor->getBrushInputInfo(input);
    auto inputMapping = mEditor->getBrushInputMapping(mSettingType, input);

    MPInputButton* removeActionButton = new MPInputButton(input, nullptr);
    removeActionButton->setIcon(QIcon(":/app/icons/new/trash-changes.png"));
    removeActionButton->setContentsMargins(5,0,5,0);

    int row =  mGridLayout->rowCount();

    settingDescLabel->setToolTip(inputInfo.tooltip);

    MPMappingWidget* mappingWidget = new MPMappingWidget(getBrushInputName(input), inputInfo.tooltip, inputInfo.soft_min, inputInfo.soft_max, input, inputMapping.controlPoints.points, 8, this);
    mappingWidget->setMinimumHeight(150);
    mappingWidget->setMaximumHeight(200);
    mVBoxLayout->addWidget(mappingWidget);

    mMappingConnections << connect(mappingWidget, &MPMappingWidget::mappingForInputUpdated, this, &MPMappingOptionsWidget::mappingForInputUpdated);

    MPMappingOption option(removeActionButton, settingDescLabel, mappingWidget);
    option.inputType = input;

    mGridLayout->addWidget(option.settingDescLabel, row, 0, 1, 1);
    mGridLayout->addWidget(option.removeActionButton, row, 1, 1, 2, Qt::AlignRight);

    mMappingConnections << connect(option.removeActionButton, &MPInputButton::didPress, this, &MPMappingOptionsWidget::removeAction);

    return option;
}

void MPMappingOptionsWidget::addOptionField(int index, QString name, int value)
{
    Q_UNUSED(name)
    BrushInputType inputType = static_cast<BrushInputType>(value);
    MPMappingOption option = createMappingOption(inputType);

    mMappingOptionsComboBox->setItemEnabled(index, false);

    mOptions << option;
}

void MPMappingOptionsWidget::removeAction(BrushInputType input)
{
    // this should not be possible...
    if (mOptions.empty()) {
        return;
    }

    for (int i = 0; i < mOptions.count(); i++) {

        MPMappingOption& option = mOptions[i];
        if (input == mOptions[i].inputType) {

            option.deleteAll();
            mOptions.removeAt(i);
            break;
        }
    }

    int index = static_cast<int>(input);
    mMappingOptionsComboBox->setItemEnabled(index, true);

    emit removedInputOption(input);
}

void MPMappingOptionsWidget::updateMappingWidgetForInput(BrushInputType)
{
    for (int i = 0; i < mOptions.count(); i++) {
        MPMappingOption& option = mOptions[i];

        // Cleanup options before clearing list
        option.deleteAll();

        // Re-enable combo options again
        mMappingOptionsComboBox->setItemEnabled(i, true);
    }
    if (!mOptions.isEmpty()) {
        mOptions.clear();
    }

    for (auto connection : mMappingConnections) {
        disconnect(mMappingConnections.takeFirst());
    }

    for (int i = 0; i < static_cast<int>(BrushInputType::BRUSH_INPUTS_COUNT); i++) {

        BrushInputType input = static_cast<BrushInputType>(i);

        auto inputMapping = mEditor->getBrushInputMapping(mSettingType, input);

        if (inputMapping.controlPoints.numberOfPoints > 0) {

            MPMappingOption option = createMappingOption(input);
            mMappingOptionsComboBox->setItemEnabled(i, false);

            mOptions << option;
        }
    }
}

void MPMappingOptionsWidget::updateMappingForInput(QVector<QPointF> points, BrushInputType inputType)
{
    emit mappingForInputUpdated(points, inputType);
}

