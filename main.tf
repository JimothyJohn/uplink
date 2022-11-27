resource "aws_iot_thing" "uptime" {
  name = "alpha"

  attributes = {
    First = "FirstUnit"
  }
}

resource "aws_iot_certificate" "cert" {
  active = true
}

data "aws_caller_identity" "current" {}

locals {
    account_id = data.aws_caller_identity.current.account_id
    thing_name = aws_iot_thing.uptime.name
    region = "us-east-1" 
}

data "aws_iam_policy_document" "uptime_policy_doc" {
  statement {
    effect = "Allow"
    actions = ["iot:*"]
    resources = ["*",]
  }
  
  /*
  statement {
    effect = "Allow"
    actions = ["iot:Connect",]
    resources = ["arn:aws:iot:${local.region}:${local.account_id}:thing/${local.thing_name}",]
  }
  
  statement {
    effect = "Allow"
    actions = ["iot:Receive",]
    resources = ["arn:aws:iot:${local.region}:${local.account_id}:topicfilter/esp32/sub",]
  }

  statement {
    effect = "Allow"
    actions = ["iot:Subscribe",]
    resources = ["arn:aws:iot:${local.region}:${local.account_id}:topic/esp32/sub",]
  }

  statement {
    effect = "Allow"
    actions = ["iot:Publish",]
    resources = ["arn:aws:iot:${local.region}:${local.account_id}:topic/esp32/pub",]
  }
  */
}

resource "aws_iot_policy" "uptime_policy" {
  name = "uptime_policy"
  policy = data.aws_iam_policy_document.uptime_policy_doc.json
}

resource "aws_iot_policy_attachment" "att" {
  policy = aws_iot_policy.uptime_policy.name
  target = aws_iot_certificate.cert.arn
}

resource "aws_iot_thing_principal_attachment" "att" {
  principal = aws_iot_certificate.cert.arn
  thing     = aws_iot_thing.uptime.name
}
