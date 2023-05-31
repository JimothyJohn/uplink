data "aws_caller_identity" "current" {}

locals {
  arn = "arn:aws:iot:${var.region}:${data.aws_caller_identity.current.account_id}"
}

data "aws_iam_policy_document" "uptime_policy_doc" {
  statement {
    effect    = "Allow"
    actions   = ["iot:Connect", ]
    resources = ["${local.arn}:client/${var.thing_name}", ]
  }

  statement {
    effect    = "Allow"
    actions   = ["iot:Publish", ]
    resources = ["${local.arn}:topic/uptime/*", ]
  }

  statement {
    effect    = "Allow"
    actions   = ["iot:Receive", ]
    resources = ["${local.arn}:topic/uptime/*", ]
  }

  statement {
    effect    = "Allow"
    actions   = ["iot:Subscribe", ]
    resources = ["${local.arn}:topicfilter/uptime/*", ]
  }
}
