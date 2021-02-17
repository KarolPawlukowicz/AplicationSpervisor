object Form1: TForm1
  Left = 192
  Top = 125
  Width = 445
  Height = 300
  Caption = 'ApplicationSupervisor'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object startBtn: TButton
    Left = 0
    Top = 0
    Width = 425
    Height = 129
    Caption = 'START'
    TabOrder = 0
    OnClick = startBtnClick
  end
  object stopBtn: TButton
    Left = 0
    Top = 128
    Width = 425
    Height = 129
    Caption = 'STOP'
    TabOrder = 1
    OnClick = stopBtnClick
  end
end
