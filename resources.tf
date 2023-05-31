resource "aws_iot_thing" "uptime" {
  name = var.thing_name

  attributes = {
    First = "FirstUnit"
  }
}

resource "aws_iot_certificate" "cert" {
  active = true
}

resource "aws_iot_policy" "uptime_policy" {
  name   = var.policy_name
  policy = data.aws_iam_policy_document.uptime_policy_doc.json
}

resource "aws_iot_policy_attachment" "att" {
  policy = var.policy_name
  target = aws_iot_certificate.cert.arn
}

resource "aws_iot_thing_principal_attachment" "att" {
  principal = aws_iot_certificate.cert.arn
  thing     = var.thing_name
}
